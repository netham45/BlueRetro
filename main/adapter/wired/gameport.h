#ifndef _GAMEPORT_H_
#define _GAMEPORT_H_
#include "adapter/config.h"

struct gameport_data {
	bool buttons[4]; //1 2 3 4
	uint8_t axis[4]; //X1 Y1 X2 Y2
};

void gameport_meta_init(struct generic_ctrl *ctrl_data);
void gameport_from_generic(int32_t dev_mode, struct generic_ctrl *ctrl_data, struct wired_data *wired_data);

#endif /* _GAMEPORT_H_ */
