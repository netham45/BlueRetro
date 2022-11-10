#include "gameport.h"
#include <string.h>
#include "wired/gameport_spi.h"
#include <esp32/rom/ets_sys.h>

#define NUM_DIGITAL_BUTTONS 4
#define NUM_8BIT_AXIS 4
#define BUTTON_MASK_SIZE 32

enum { // Digital buttons
    GAMEPAD_1,
	GAMEPAD_2,
	GAMEPAD_3,
	GAMEPAD_4
};


static DRAM_ATTR const int gameport_btns_mask[BUTTON_MASK_SIZE] = {
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    GAMEPAD_1, GAMEPAD_2, GAMEPAD_3, GAMEPAD_4,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
};

static DRAM_ATTR const struct ctrl_meta gameport_axes_meta[ADAPTER_MAX_AXES] =
{
    {.size_min = -128, .size_max = 127, .neutral = 0, .abs_max = 127},
    {.size_min = -128, .size_max = 127, .neutral = 0, .abs_max = 127},
    {.size_min = -128, .size_max = 127, .neutral = 0, .abs_max = 127},
    {.size_min = -128, .size_max = 127, .neutral = 0, .abs_max = 127},
	{.size_min = 0, .size_max = 0, .neutral = 0x00, .abs_max = 0}, // Unused
    {.size_min = 0, .size_max = 0, .neutral = 0x00, .abs_max = 0}
};

static const uint32_t gameport_mask[4] = {0xBBFF0FFF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t gameport_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};

void gameport_meta_init(struct generic_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            switch (config.out_cfg[i].dev_mode) {
                case DEV_PAD_ALT:
                    ctrl_data[i].mask = gameport_mask;
                    ctrl_data[i].desc = gameport_desc;
                    ctrl_data[i].axes[j].meta = &gameport_axes_meta[j];
                    break;
                default:
                    ctrl_data[i].mask = gameport_mask;
                    ctrl_data[i].desc = gameport_desc;
                    ctrl_data[i].axes[j].meta = &gameport_axes_meta[j];
                    break;
            }
        }
    }
}


void gameport_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {    
    struct gameport_data *data = (struct gameport_data*) wired_data->output;

    for (int i=0;i<BUTTON_MASK_SIZE;i++)
    {
        if (gameport_btns_mask[i] != -1)
        {
			data->buttons[gameport_btns_mask[i]] = (ctrl_data->btns[0].value & (1<<i));
        }
    }
    
    for (int i=0;i<NUM_8BIT_AXIS;i++) // 8 bit axis
    {
        if (ctrl_data->axes[i].value > ctrl_data->axes[i].meta->size_max) {
            data->axis[i] = 255;
        }
        else if (ctrl_data->axes[i].value < ctrl_data->axes[i].meta->size_min) {
            data->axis[i] = 0;
        }
        else {     
            data->axis[i] = ctrl_data->axes[i].value + 128;
        }
    }
	gameport_process();
}