/*
 * Copyright (c) 2019-2022, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "adapter/config.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "ogx360.h"

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
    return;
    memset(&data,0,sizeof(struct usbd_duke_t));
}

void ogx360_meta_init(struct generic_ctrl *ctrl_data) {

}

void ogx360_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {
	return;
	memset(&data,0,sizeof(data));
	data.out.startByte = 0xF1;
	data.out.bLength = sizeof(data.out);
	data.out.wButtons = ctrl_data->map_mask[0] & 0xFF;
	data.out.WHITE = (ctrl_data->map_mask[0] & BIT(8)) > 0 ? 0xFF : 0;
	data.out.BLACK = (ctrl_data->map_mask[0] & BIT(9)) > 0 ? 0xFF : 0;
	data.out.A = (ctrl_data->map_mask[0] & BIT(10)) > 0 ? 0xFF : 0;
	data.out.B = (ctrl_data->map_mask[0] & BIT(11)) > 0 ? 0xFF : 0;
	data.out.X = (ctrl_data->map_mask[0] & BIT(12)) > 0 ? 0xFF : 0;
	data.out.Y = (ctrl_data->map_mask[0] & BIT(13)) > 0 ? 0xFF : 0;

	for (int i=0;i<6;i++)
	{
		int16_t result = 0;
		if (ctrl_data->map_mask[0] & (axis_to_btn_mask(0))) {
			if (ctrl_data->axes[i].value > 32767) {
				result = 32767;
			}
			else if (ctrl_data->axes[0].value < -32768) {
				result = -32768;
			}
			else {
				result = ctrl_data->axes[0].value;
			}
		}
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
				data.out.L = result;
				break;
			case 5:
				data.out.R = result;
				break;
		}
	}
	memcpy(wired_data->output, (void *)&data, sizeof(data));
}

void ogx360_gen_turbo_mask(struct wired_data *wired_data) {
	
}
