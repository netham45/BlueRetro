/*
 * Copyright (c) 2019-2022, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OGX360_H_
#define _OGX360_H_
#include "adapter/adapter.h"

void ogx360_meta_init(struct generic_ctrl *ctrl_data);
void ogx360_init_buffer(int32_t dev_mode, struct wired_data *wired_data);
void ogx360_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data);
void ogx360_gen_turbo_mask(struct wired_data *wired_data);

#endif /* _OGX360_H_ */
