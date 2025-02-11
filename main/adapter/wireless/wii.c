/*
 * Copyright (c) 2019-2020, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "zephyr/types.h"
#include "tools/util.h"
#include "wii.h"

#define WIIU_AXES_MAX 4
#define NUNCHUCK_AXES_MAX 2
enum {
    WII_CORE_D_LEFT = 0,
    WII_CORE_D_RIGHT,
    WII_CORE_D_DOWN,
    WII_CORE_D_UP,
    WII_CORE_PLUS,
    WII_CORE_2 = 8,
    WII_CORE_1,
    WII_CORE_B,
    WII_CORE_A,
    WII_CORE_MINUS,
    WII_CORE_HOME = 15,
};

enum {
    WII_CLASSIC_R = 1,
    WII_CLASSIC_PLUS,
    WII_CLASSIC_HOME,
    WII_CLASSIC_MINUS,
    WII_CLASSIC_L,
    WII_CLASSIC_D_DOWN,
    WII_CLASSIC_D_RIGHT,
    WII_CLASSIC_D_UP,
    WII_CLASSIC_D_LEFT,
    WII_CLASSIC_ZR,
    WII_CLASSIC_X,
    WII_CLASSIC_A,
    WII_CLASSIC_Y,
    WII_CLASSIC_B,
    WII_CLASSIC_ZL,
};

enum {
    WII_NUNCHUCK_Z = 0,
    WII_NUNCHUCK_C,
};

enum {
    WIIU_R = 1,
    WIIU_PLUS,
    WIIU_HOME,
    WIIU_MINUS,
    WIIU_L,
    WIIU_D_DOWN,
    WIIU_D_RIGHT,
    WIIU_D_UP,
    WIIU_D_LEFT,
    WIIU_ZR,
    WIIU_X,
    WIIU_A,
    WIIU_Y,
    WIIU_B,
    WIIU_ZL,
    WIIU_RJ,
    WIIU_LJ,
};

struct wiic_map {
    uint16_t core;
    uint8_t reserved[3];
    uint8_t axes[4];
    uint16_t buttons;
} __packed;

struct wiin_map {
    uint16_t core;
    uint8_t reserved[3];
    uint8_t axes[5];
    uint8_t buttons;
} __packed;

struct wiiu_map {
    uint8_t reserved[5];
    uint16_t axes[4];
    uint32_t buttons;
} __packed;

static const uint8_t led_dev_id_map[] = {
    0x1, 0x2, 0x4, 0x8, 0x3, 0x6, 0xC
};

static const uint8_t wiiu_axes_idx[WIIU_AXES_MAX] =
{
/*  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY  */
    0,       2,       1,       3
};

static const struct ctrl_meta wiin_axes_meta[NUNCHUCK_AXES_MAX] =
{
    {.neutral = 0x80, .abs_max = 0x63},
    {.neutral = 0x80, .abs_max = 0x63},
};

static const struct ctrl_meta wiic_axes_meta[ADAPTER_MAX_AXES] =
{
    {.neutral = 0x20, .abs_max = 0x1B},
    {.neutral = 0x20, .abs_max = 0x1B},
    {.neutral = 0x10, .abs_max = 0x0D},
    {.neutral = 0x10, .abs_max = 0x0D},
    {.neutral = 0x02, .abs_max = 0x1D},
    {.neutral = 0x02, .abs_max = 0x1D},
};

static const struct ctrl_meta wiiu_axes_meta =
{
    .neutral = 0x800,
    .abs_max = 0x44C,
};

static const uint32_t wii_mask[4] = {0x007F0F00, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wii_desc[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiin_mask[4] = {0x13770F0F, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiin_desc[4] = {0x0000000F, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiic_mask[4] = {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiic_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiiu_mask[4] = {0xBB7F0FFF, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t wiiu_desc[4] = {0x000000FF, 0x00000000, 0x00000000, 0x00000000};

static const uint32_t wii_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WII_CORE_D_UP), BIT(WII_CORE_D_DOWN), BIT(WII_CORE_D_LEFT), BIT(WII_CORE_D_RIGHT),
    0, 0, 0, 0,
    BIT(WII_CORE_1), BIT(WII_CORE_B), BIT(WII_CORE_2), BIT(WII_CORE_A),
    BIT(WII_CORE_PLUS), BIT(WII_CORE_MINUS), BIT(WII_CORE_HOME), 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
static const uint32_t wiin_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WII_NUNCHUCK_Z), BIT(WII_NUNCHUCK_C), 0, 0,
    0, 0, 0, 0,
};
static const uint32_t wiin_core_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WII_CORE_D_LEFT), BIT(WII_CORE_D_RIGHT), BIT(WII_CORE_D_DOWN), BIT(WII_CORE_D_UP),
    0, 0, 0, 0,
    BIT(WII_CORE_1), BIT(WII_CORE_2), BIT(WII_CORE_A), 0,
    BIT(WII_CORE_PLUS), BIT(WII_CORE_MINUS), BIT(WII_CORE_HOME), 0,
    0, 0, 0, 0,
    BIT(WII_CORE_B), 0, 0, 0,
};
static const uint32_t wiic_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WII_CLASSIC_D_LEFT), BIT(WII_CLASSIC_D_RIGHT), BIT(WII_CLASSIC_D_DOWN), BIT(WII_CLASSIC_D_UP),
    0, 0, 0, 0,
    BIT(WII_CLASSIC_Y), BIT(WII_CLASSIC_A), BIT(WII_CLASSIC_B), BIT(WII_CLASSIC_X),
    BIT(WII_CLASSIC_PLUS), BIT(WII_CLASSIC_MINUS), BIT(WII_CLASSIC_HOME), 0,
    0, BIT(WII_CLASSIC_ZL), BIT(WII_CLASSIC_L), 0,
    0, BIT(WII_CLASSIC_ZR), BIT(WII_CLASSIC_R), 0,
};
static const uint32_t wiic_core_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WII_CORE_D_UP), BIT(WII_CORE_D_DOWN), BIT(WII_CORE_D_LEFT), BIT(WII_CORE_D_RIGHT),
    0, 0, 0, 0,
    0, 0, 0, BIT(WII_CORE_B),
    0, 0, 0, BIT(WII_CORE_1),
    0, 0, 0, BIT(WII_CORE_2),
};
static const uint32_t wiiu_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(WIIU_D_LEFT), BIT(WIIU_D_RIGHT), BIT(WIIU_D_DOWN), BIT(WIIU_D_UP),
    0, 0, 0, 0,
    BIT(WIIU_Y), BIT(WIIU_A), BIT(WIIU_B), BIT(WIIU_X),
    BIT(WIIU_PLUS), BIT(WIIU_MINUS), BIT(WIIU_HOME), 0,
    BIT(WIIU_ZL), BIT(WIIU_L), 0, BIT(WIIU_LJ),
    BIT(WIIU_ZR), BIT(WIIU_R), 0, BIT(WIIU_RJ),
};

static int32_t wiimote_to_generic(struct bt_data *bt_data, struct generic_ctrl *ctrl_data) {
    uint16_t *buttons = (uint16_t *)bt_data->input;

    memset((void *)ctrl_data, 0, sizeof(*ctrl_data));

    ctrl_data->mask = (uint32_t *)wii_mask;
    ctrl_data->desc = (uint32_t *)wii_desc;

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (*buttons & wii_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    return 0;
}

static int32_t wiin_to_generic(struct bt_data *bt_data, struct generic_ctrl *ctrl_data) {
    struct wiin_map *map = (struct wiin_map *)bt_data->input;

    memset((void *)ctrl_data, 0, sizeof(*ctrl_data));

    ctrl_data->mask = (uint32_t *)wiin_mask;
    ctrl_data->desc = (uint32_t *)wiin_desc;

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (map->core & wiin_core_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (~map->buttons & wiin_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    if (!atomic_test_bit(&bt_data->flags, BT_INIT)) {
        for (uint32_t i = 0; i < NUNCHUCK_AXES_MAX; i++) {
            bt_data->axes_cal[i] = -(map->axes[i] - wiin_axes_meta[i].neutral);
        }
        atomic_set_bit(&bt_data->flags, BT_INIT);
    }

    for (uint32_t i = 0; i < NUNCHUCK_AXES_MAX; i++) {
        ctrl_data->axes[i].meta = &wiin_axes_meta[i];
        ctrl_data->axes[i].value = map->axes[i] - wiin_axes_meta[i].neutral + bt_data->axes_cal[i];
    }

    return 0;
}

static int32_t wiic_to_generic(struct bt_data *bt_data, struct generic_ctrl *ctrl_data) {
    struct wiic_map *map = (struct wiic_map *)bt_data->input;
    uint8_t axes[6];

    memset((void *)ctrl_data, 0, sizeof(*ctrl_data));

    ctrl_data->mask = (uint32_t *)wiic_mask;
    ctrl_data->desc = (uint32_t *)wiic_desc;

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (map->core & wiic_core_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (~map->buttons & wiic_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    axes[0] = map->axes[0] & 0x3F;
    axes[1] = map->axes[1] & 0x3F;
    axes[2] = ((map->axes[0] & 0xC0) >> 3) | ((map->axes[1] & 0xC0) >> 5) | ((map->axes[2] & 0x80) >> 7);
    axes[3] = map->axes[2] & 0x1F;
    axes[4] = ((map->axes[2] & 0x60) >> 2) | ((map->axes[3] & 0xE0) >> 5);
    axes[5] = map->axes[3] & 0x1F;

    if (!atomic_test_bit(&bt_data->flags, BT_INIT)) {
        for (uint32_t i = 0; i < ADAPTER_MAX_AXES; i++) {
            bt_data->axes_cal[i] = -(axes[i] - wiic_axes_meta[i].neutral);
        }
        atomic_set_bit(&bt_data->flags, BT_INIT);
    }

    for (uint32_t i = 0; i < ADAPTER_MAX_AXES; i++) {
        ctrl_data->axes[i].meta = &wiic_axes_meta[i];
        ctrl_data->axes[i].value = axes[i] - wiic_axes_meta[i].neutral + bt_data->axes_cal[i];
    }

    return 0;
}

static int32_t wiiu_to_generic(struct bt_data *bt_data, struct generic_ctrl *ctrl_data) {
    struct wiiu_map *map = (struct wiiu_map *)bt_data->input;

    memset((void *)ctrl_data, 0, sizeof(*ctrl_data));

    ctrl_data->mask = (uint32_t *)wiiu_mask;
    ctrl_data->desc = (uint32_t *)wiiu_desc;

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (~map->buttons & wiiu_btns_mask[i]) {
            ctrl_data->btns[0].value |= generic_btns_mask[i];
        }
    }

    if (!atomic_test_bit(&bt_data->flags, BT_INIT)) {
        for (uint32_t i = 0; i < WIIU_AXES_MAX; i++) {
            bt_data->axes_cal[i] = -(map->axes[wiiu_axes_idx[i]] - wiiu_axes_meta.neutral);
        }
        atomic_set_bit(&bt_data->flags, BT_INIT);
    }

    for (uint32_t i = 0; i < WIIU_AXES_MAX; i++) {
        ctrl_data->axes[i].meta = &wiiu_axes_meta;
        ctrl_data->axes[i].value = map->axes[wiiu_axes_idx[i]] - wiiu_axes_meta.neutral + bt_data->axes_cal[i];
    }

    return 0;
}

int32_t wii_to_generic(struct bt_data *bt_data, struct generic_ctrl *ctrl_data) {
    switch (bt_data->pids->subtype) {
        case BT_WII_NUNCHUCK:
            return wiin_to_generic(bt_data, ctrl_data);
        case BT_WII_CLASSIC:
            return wiic_to_generic(bt_data, ctrl_data);
        case BT_WIIU_PRO:
            return wiiu_to_generic(bt_data, ctrl_data);
        default:
            return wiimote_to_generic(bt_data, ctrl_data);
    }
}

void wii_fb_from_generic(struct generic_fb *fb_data, struct bt_data *bt_data) {
    if (fb_data->state) {
        bt_data->output[0] = (led_dev_id_map[bt_data->pids->id] << 4) | 0x01;
    }
    else {
        bt_data->output[0] = (led_dev_id_map[bt_data->pids->id] << 4);
    }
}
