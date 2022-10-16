#include "ogx360_i2c.h"
#include "driver/i2c.h"
#include "adapter/adapter.h"
#include "adapter/config.h"
#include <freertos/task.h>
#include <esp32/rom/ets_sys.h>

bool players[4] = {0};

typedef struct __attribute__((packed)) usbd_duke_in
{
    uint8_t startByte;
    uint8_t bLength;
    uint16_t lValue;
    uint16_t hValue;
} usbd_duke_in_t;

void ogx360_check_connected_controllers()
{
	ets_printf("Checking controllers\n");
	for (int i=0;i<4;i++)
	{
		const char ping[] = { 0xAA };
		esp_err_t result = i2c_master_write_to_device(I2C_NUM_0, i + 1, (void*)ping, sizeof(ping), 150);
		players[i] = (result == ESP_OK);
		ets_printf("OGX360_I2C: Player %d %s\n",i,players[i]?"Found":"Not Found");
	}
	ets_printf("Checking controllers done\n");
}

void ogx360_initialize_i2c(void) {
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 22,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = 21,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
		.clk_flags = 0,
	};
	ets_printf("ogx360_init_i2c 1\n");
	i2c_param_config(I2C_NUM_0, &conf);
	ets_printf("ogx360_init_i2c 2\n");
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	ets_printf("ogx360_init_i2c 3\n");
}

bool initialized = false;

void ogx360_init(void)
{
	struct usbd_duke_in duke_in = { 0 };
	if (!initialized)
	{
		ogx360_initialize_i2c();
		ogx360_check_connected_controllers();
		initialized = true;
	}
	
//	while(1)
	{
		ets_printf("ogx360_loop\n");
		for (int i=0;i<4;i++)
		{
			if (players[i])
			{
				i2c_master_write_to_device(I2C_NUM_0, i + 1, (void*)&wired_adapter.data[i].output, 21, 150);
				i2c_master_read_from_device(I2C_NUM_0,i + 1, (void*)&duke_in, sizeof(duke_in), 150);
			}
		}
	}
}