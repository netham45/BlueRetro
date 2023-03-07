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

static const uint32_t gameport_kb_mask[4] = {0xE6FF0F0F, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static const uint32_t gameport_kb_desc[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
static const uint8_t gameport_kb_scancode[KBM_MAX] = {
 /* KB_A, KB_D, KB_S, KB_W, MOUSE_X_LEFT, MOUSE_X_RIGHT, MOUSE_Y_DOWN MOUSE_Y_UP */
    KEY_A, KEY_D, KEY_S, KEY_W, 0x00, 0x00, 0x00, 0x00,
 /* KB_LEFT, KB_RIGHT, KB_DOWN, KB_UP, MOUSE_WX_LEFT, MOUSE_WX_RIGHT, MOUSE_WY_DOWN, MOUSE_WY_UP */
    KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, 0x00, 0x00, 0x00, 0x00,
 /* KB_Q, KB_R, KB_E, KB_F, KB_ESC, KB_ENTER, KB_LWIN, KB_HASH */
    KEY_Q, KEY_R, KEY_E, KEY_F, KEY_ESC, KEY_ENTER, KEY_LEFTMETA, 0,
 /* MOUSE_RIGHT, KB_Z, KB_LCTRL, MOUSE_MIDDLE, MOUSE_LEFT, KB_X, KB_LSHIFT, KB_SPACE */
    0x00, KEY_Z, KEY_LEFTCTRL, 0x00, 0x00, KEY_X, KEY_LEFTSHIFT, KEY_SPACE,
 /* KB_B, KB_C, KB_G, KB_H, KB_I, KB_J, KB_K, KB_L */
    KEY_B, KEY_C, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L,
 /* KB_M, KB_N, KB_O, KB_P, KB_T, KB_U, KB_V, KB_Y */
    KEY_M, KEY_N, KEY_O, KEY_P, KEY_T, KEY_U, KEY_V, KEY_Y,
 /* KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8 */
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8,
 /* KB_9, KB_0, KB_BACKSPACE, KB_TAB, KB_MINUS, KB_EQUAL, KB_LEFTBRACE, KB_RIGHTBRACE */
    KEY_9, KEY_0, KEY_BACKSPACE, KEY_TAB, KEY_MINUS, KEY_EQUAL, KEY_LEFTBRACE, KEY_RIGHTBRACE,
 /* KB_BACKSLASH, KB_SEMICOLON, KB_APOSTROPHE, KB_GRAVE, KB_COMMA, KB_DOT, KB_SLASH, KB_CAPSLOCK */
    KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_CAPSLOCK,
 /* KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6, KB_F7, KB_F8 */
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
 /* KB_F9, KB_F10, KB_F11, KB_F12, KB_PSCREEN, KB_SCROLL, KB_PAUSE, KB_INSERT */
    KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_SYSRQ, KEY_SCROLLLOCK, KEY_PAUSE, KEY_INSERT,
 /* KB_HOME, KB_PAGEUP, KB_DEL, KB_END, KB_PAGEDOWN, KB_NUMLOCK, KB_KP_DIV, KB_KP_MULTI */
    KEY_HOME, KEY_PAGEUP, KEY_DELETE, KEY_END, KEY_PAGEDOWN, KEY_NUMLOCK, KEY_KPSLASH, KEY_KPASTERISK,
 /* KB_KP_MINUS, KB_KP_PLUS, KB_KP_ENTER, KB_KP_1, KB_KP_2, KB_KP_3, KB_KP_4, KB_KP_5 */
    KEY_KPMINUS, KEY_KPPLUS, KEY_KPENTER, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5,
 /* KB_KP_6, KB_KP_7, KB_KP_8, KB_KP_9, KB_KP_0, KB_KP_DOT, KB_LALT, KB_RCTRL */
    KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP0, KEY_KPDOT, KEY_LEFTALT, KEY_RIGHTCTRL,
 /* KB_RSHIFT, KB_RALT, KB_RWIN */
    KEY_RIGHTSHIFT, KEY_RIGHTALT, KEY_RIGHTMETA,
};

bool buttonPressed[256] = {0};


void gameport_meta_init(struct generic_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            switch (config.out_cfg[i].dev_mode) {
                case DEV_KB:
                    ctrl_data[i].mask = gameport_kb_mask;
                    ctrl_data[i].desc = gameport_kb_desc;
                    break;
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
    data->magic = 'g';
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
	gameport_process(wired_data->index);
}

static void gameport_kb_from_generic(struct generic_ctrl *ctrl_data, struct wired_data *wired_data) {
    struct gameport_kb_data *data = (struct gameport_kb_data*) wired_data->output;
    data->magic = 'k';
    for (uint32_t i = 0; i < KBM_MAX; i++) {
        data->key = gameport_kb_scancode[i];
        data->pressed = ctrl_data->btns[i / 32].value & (1<<(i & 0x1F));
        if (buttonPressed[i] != data->pressed) {
            buttonPressed[i] = data->pressed;
            break;
        }
    }
    gameport_process(wired_data->index);
}