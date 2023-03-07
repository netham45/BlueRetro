#include "gameport.h"
#include <string.h>
#include "wired/gameport_spi.h"
#include <esp32/rom/ets_sys.h>
#include "zephyr/types.h"
#include "tools/util.h"

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

static const uint32_t gameport_kb_mask[4] = {0xE6FF0F0F, 0xFFFFFFFF, 0xFFFFFFFF, 0x0007FFFF};
static const uint32_t gameport_kb_desc[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};



void gameport_meta_init(struct generic_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            switch (config.out_cfg[i].dev_mode) {
                case DEV_KB:
                    ctrl_data[i].mask = gameport_kb_mask;
                    ctrl_data[i].desc = gameport_kb_desc;
                    ctrl_data[i].axes[j].meta = 0;
                    goto exit_axes_loop;
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
exit_axes_loop:
        ;
    }
}

void gameport_ctrl_from_generic(struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {   
     
    struct gameport_data *data = (struct gameport_data*) wired_data->output;
    data->magic = 'g';
    for (int i=0;i<BUTTON_MASK_SIZE;i++)
    {
        if (gameport_btns_mask[i] != -1)
        {
			data->buttons[gameport_btns_mask[i]] = (ctrl_data->btns[0].value & BIT(i));
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
}

static void gameport_kb_from_generic(struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {
    struct gameport_kb_data *data = (struct gameport_kb_data*) wired_data->output;
    data->magic = 'k';
    for (int i=0;i<4;i++)
    {
        data->buttons[i] = ctrl_data->btns[i].value;
    }
}

void gameport_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {   
    switch (dev_mode) {
        case DEV_KB:
            gameport_kb_from_generic(ctrl_data, wired_data);
            break;
        case DEV_MOUSE:
            //gameport_mouse_from_generic(ctrl_data, wired_data);
            break;
        case DEV_PAD:
        default:
            gameport_ctrl_from_generic(ctrl_data, wired_data);
            break;
    }
    gameport_process(wired_data->index, ctrl_data->input_index);
}