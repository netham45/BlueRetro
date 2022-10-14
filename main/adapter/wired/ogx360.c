/*
 * Copyright (c) 2019-2022, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "adapter/config.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "ogx360.h"
#include "soc/io_mux_reg.h"
#include "esp_private/periph_ctrl.h"
#include <soc/i2c_periph.h>
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/gpio.h>
#include "hal/i2c_ll.h"
#include "hal/clk_gate_ll.h"
#include "hal/misc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "system/intr.h"
#include "system/gpio.h"
#include "system/delay.h"
#include "zephyr/atomic.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "adapter/adapter.h"
#include "adapter/config.h"

enum {
	OGX360_D_UP,
	OGX360_D_DOWN,
	OGX360_D_LEFT,
	OGX360_D_RIGHT,
	OGX360_START,
	OGX360_BACK,
	OGX360_WHITE,
	OGX360_BLACK,
	OGX360_A,
	OGX360_B,
	OGX360_X,
	OGX360_Y,
	OGX360_L,
	OGX360_R,
};

/*static DRAM_ATTR const uint32_t ogx360_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(OGX360_D_LEFT), BIT(OGX360_D_RIGHT), BIT(OGX360_D_DOWN), BIT(OGX360_D_UP),
    0, 0, 0, 0,
    BIT(OGX360_X), BIT(OGX360_B), BIT(OGX360_A), BIT(OGX360_Y),
    BIT(OGX360_START), BIT(OGX360_BACK), 0, 0,
    BIT(OGX360_L), BIT(OGX360_BLACK), BIT(OGX360_L), 0,
    BIT(OGX360_R), BIT(OGX360_WHITE), BIT(OGX360_R), 0,
};*/

static DRAM_ATTR const struct ctrl_meta ogx360_axes_meta[ADAPTER_MAX_AXES] =
{
    {.size_min = -128, .size_max = 127, .neutral = 0x80, .abs_max = 0x66},
    {.size_min = -128, .size_max = 127, .neutral = 0x80, .abs_max = 0x66},
    {.size_min = -128, .size_max = 127, .neutral = 0x80, .abs_max = 0x66},
    {.size_min = -128, .size_max = 127, .neutral = 0x80, .abs_max = 0x66},
    {.size_min = 0, .size_max = 255, .neutral = 0x16, .abs_max = 0xDA},
    {.size_min = 0, .size_max = 255, .neutral = 0x16, .abs_max = 0xDA},
};

struct ogx360_map {
    uint8_t axes[6];
    uint16_t buttons;
} __packed;

/*static DRAM_ATTR const uint8_t ogx360_axes_idx[ADAPTER_MAX_AXES] =
{
//  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, TRIG_L, TRIG_R
    0,       2,       1,       3,       4,      5
};*/

static const uint32_t ogx360_mask[4] = {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t ogx360_pro_desc[4] = {0x000000FF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t ogx360_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};


struct usbd_duke_out_t
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t wButtons;
    uint8_t A;
    uint8_t B;
    uint8_t X;
    uint8_t Y;
    uint8_t BLACK;
    uint8_t WHITE;
    uint8_t L;
    uint8_t R;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
};

struct usbd_duke_in_t
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t lValue;
    uint16_t hValue;
};

struct usbd_duke_t
{
    struct usbd_duke_in_t in;
    struct usbd_duke_out_t out;
};

struct usbd_duke_t data;


void IRAM_ATTR ogx360_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    memset(&data,0,sizeof(struct usbd_duke_t));
}

void ogx360_meta_init(struct generic_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            switch (config.out_cfg[i].dev_mode) {
                case DEV_PAD_ALT:
                    ctrl_data[i].mask = ogx360_mask;
                    ctrl_data[i].desc = ogx360_desc;
                    ctrl_data[i].axes[j].meta = &ogx360_axes_meta[j];
                    break;
                default:
                    ctrl_data[i].mask = ogx360_mask;
                    ctrl_data[i].desc = ogx360_pro_desc;
                    ctrl_data[i].axes[j].meta = &ogx360_axes_meta[j];
                    break;
            }
        }
    }
}

bool i2c_installed = false;

void ogx360_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {

	
	if (!i2c_installed)
	{
		i2c_installed = true;
		int i2c_master_port = 0;
		i2c_config_t conf = {
			.mode = I2C_MODE_MASTER,
			.sda_io_num = 22,
			.sda_pullup_en = GPIO_PULLUP_ENABLE,
			.scl_io_num = 21,
			.scl_pullup_en = GPIO_PULLUP_ENABLE,
			.master.clk_speed = 100000,
			.clk_flags = 0,
		};
		i2c_param_config(I2C_NUM_0, &conf);
		i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	}
	memset(&data,0,sizeof(data));
	data.out.startByte = 0xF1;
	data.out.bLength = sizeof(data.out);
	if (ctrl_data->btns[0].value & BIT(11)) data.out.wButtons |= BIT(0);
	if (ctrl_data->btns[0].value & BIT(10)) data.out.wButtons |= BIT(1);
	if (ctrl_data->btns[0].value & BIT(8)) data.out.wButtons |= BIT(2);
	if (ctrl_data->btns[0].value & BIT(9)) data.out.wButtons |= BIT(3);
	if (ctrl_data->btns[0].value & BIT(20)) data.out.wButtons |= BIT(4);
	if (ctrl_data->btns[0].value & BIT(21)) data.out.wButtons |= BIT(5);
	if (ctrl_data->btns[0].value & BIT(26)) data.out.wButtons |= BIT(6);
	if (ctrl_data->btns[0].value & BIT(30)) data.out.wButtons |= BIT(7);
	data.out.A |= (ctrl_data->btns[0].value & BIT(18)) > 0 ? 0xFF : 0;
	data.out.B |= (ctrl_data->btns[0].value & BIT(17)) > 0 ? 0xFF : 0;
	data.out.X |= (ctrl_data->btns[0].value & BIT(16)) > 0 ? 0xFF : 0;
	data.out.Y |= (ctrl_data->btns[0].value & BIT(19)) > 0 ? 0xFF : 0;
	data.out.WHITE |= (ctrl_data->btns[0].value & BIT(29)) > 0 ? 0xFF : 0;
	data.out.BLACK |= (ctrl_data->btns[0].value & BIT(25)) > 0 ? 0xFF : 0;

	for (int i=0;i<6;i++)
	{
		int16_t result = 0;
		//if (ctrl_data->btns[i].value & (axis_to_btn_mask(0))) {
			result = ctrl_data->axes[i].value;
		//}
		switch(i)
		{
			case 0:
				data.out.leftStickX = result;
				break;
			case 1:
				data.out.leftStickY = result;
				break;
			case 2:
				data.out.rightStickX = result;
				break;
			case 3:
				data.out.rightStickY = result;
				break;
			case 4:
				data.out.L = result / 255;
				break;
			case 5:
				data.out.R = result / 255;
				break;
		}
	}
	memcpy(wired_data->output, (void *)&data, sizeof(data));

	i2c_master_write_to_device(I2C_NUM_0, 1, &data, sizeof(data), 1000);
}

void ogx360_gen_turbo_mask(struct wired_data *wired_data) {
	
}
