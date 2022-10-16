#ifndef _OGX360_H_
#define _OGX360_H_
#include <string.h>
#include "adapter/adapter.h"
#include "adapter/config.h"

#define BUTTON_MASK_SIZE 32
#define BUTTON_MASK_SIZE 32
#define NUM_DIGITAL_BUTTONS 8
#define NUM_ANALOG_BUTTONS 6
#define NUM_8BIT_AXIS 2
#define NUM_16BIT_AXIS 4

void ogx360_meta_init(struct generic_ctrl *ctrl_data);
void ogx360_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data);

#endif /* _OGX360_H_ */
