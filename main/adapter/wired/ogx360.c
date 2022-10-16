#include "ogx360.h"
#include "wired/ogx360_i2c.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//#define BIT(x) (1<<x)
enum { // Digital buttons
    OGX360_D_UP,
    OGX360_D_DOWN,
    OGX360_D_LEFT,
    OGX360_D_RIGHT,
    OGX360_START,
    OGX360_BACK,
    OGX360_LSTICK,
    OGX360_RSTICK
};

enum { // Analog buttons
    OGX360_A,
    OGX360_B,
    OGX360_X,
    OGX360_Y,
    OGX360_BLACK,
    OGX360_WHITE,
};

static DRAM_ATTR const int ogx360_digital_btns_mask[BUTTON_MASK_SIZE] = {
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    OGX360_D_LEFT, OGX360_D_RIGHT, OGX360_D_DOWN, OGX360_D_UP,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    OGX360_START, OGX360_BACK, -1, -1,
    -1, -1, -1, OGX360_LSTICK,
    -1, -1, -1, OGX360_RSTICK
};

static DRAM_ATTR const int ogx360_analog_btns_mask[BUTTON_MASK_SIZE] = {
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    OGX360_X, OGX360_B, OGX360_A, OGX360_Y,
    -1, -1, -1, -1,
    -1, OGX360_BLACK, -1, -1,
    -1, OGX360_WHITE, -1, -1,
};

static DRAM_ATTR const struct ctrl_meta ogx360_axes_meta[ADAPTER_MAX_AXES] =
{
    {.size_min = -32253, .size_max = 32253, .neutral = 0x00, .abs_max = 32253},
    {.size_min = -32253, .size_max = 32253, .neutral = 0x00, .abs_max = 32253},
    {.size_min = -32253, .size_max = 32253, .neutral = 0x00, .abs_max = 32253},
    {.size_min = -32253, .size_max = 32253, .neutral = 0x00, .abs_max = 32253},
    {.size_min = 0, .size_max = 255, .neutral = 0x00, .abs_max = 255},
    {.size_min = 0, .size_max = 255, .neutral = 0x00, .abs_max = 255},
};

static const uint32_t ogx360_mask[4] = {0xBBFF0FFF, 0x00000000, 0x00000000, 0x00000000}; //TODO: What do these exactly do?
static const uint32_t ogx360_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};

typedef struct __attribute__((packed)) usbd_duke_out
{
    uint8_t controllerType;
    uint8_t startByte;
    uint8_t bLength;
    uint16_t wButtons;
    uint8_t analogButtons[NUM_ANALOG_BUTTONS];
    uint8_t axis8[NUM_8BIT_AXIS];
    uint16_t axis16[NUM_16BIT_AXIS];
} usbd_duke_out_t;


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
                    ctrl_data[i].desc = ogx360_desc;
                    ctrl_data[i].axes[j].meta = &ogx360_axes_meta[j];
                    break;
            }
        }
    }
}

void ogx360_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {    
	struct usbd_duke_out duke_out = { 0 }; // ogx360_i2c.c
    duke_out.controllerType = 0xF1;
    duke_out.startByte = 0;
    duke_out.bLength = 6; // Always needs to be 6 according to docs
    
    for (int i=0;i<BUTTON_MASK_SIZE;i++) // Digital buttons
    {
        if (ogx360_digital_btns_mask[i] != -1)
        {
            if ((ctrl_data->btns[0].value & BIT(i)) != 0)
            {
                duke_out.wButtons |= BIT(ogx360_digital_btns_mask[i]);
            }
        }
    }
    
    for (int i=0;i<BUTTON_MASK_SIZE;i++) // Analog buttons
    {
        if (ogx360_analog_btns_mask[i] != -1)
        {
            bool pressed = (ctrl_data->btns[0].value & BIT(i)) > 0;
            if (pressed)
            {
                duke_out.analogButtons[ogx360_analog_btns_mask[i]] = 0xFF;
            }
            else
            {
                duke_out.analogButtons[ogx360_analog_btns_mask[i]] = 0x00;
            }
        }
    }
    
    for (int i=0;i<NUM_16BIT_AXIS;i++) // 16 bit axis
    {
        duke_out.axis16[i] = ctrl_data->axes[i].value;
    }
    
    for (int i=0;i<NUM_8BIT_AXIS;i++) // 8 bit axis
    {
        duke_out.axis8[i] = ctrl_data->axes[i + NUM_16BIT_AXIS].value;
    }
    memcpy(wired_data->output, (void *)&duke_out, sizeof(duke_out));
	ogx360_init(); //Hack
}