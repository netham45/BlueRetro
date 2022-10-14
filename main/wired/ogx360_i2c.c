

#include "ogx360_i2c.h"
#include "sdkconfig.h"
#include <string.h>
#include "soc/io_mux_reg.h"
#include "esp_private/periph_ctrl.h"
#include <soc/i2c_periph.h>
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/gpio.h>
#include "hal/i2c_ll.h"
#include "hal/clk_gate_ll.h"
#include "hal/misc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "system/intr.h"
#include "system/gpio.h"
#include "system/delay.h"
#include "zephyr/atomic.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "adapter/adapter.h"
#include "adapter/config.h"

#define SCL_PIN 25
#define SDA_PIN 26
#define FIFO_ADDR 0x6001301c
#define I2C0_INTR_NUM 19


void i2c_write(unsigned char* data, uint32_t len, uint8_t port)
{
	i2c_master_write_to_device(I2C_NUM_0,port,data,len,10000);
}

void ogx360_send_data()
{
	for (int i=0;i<4;i++)
	{
		uint8_t length = ((uint8_t*)wired_adapter.data[i].output)[1];
		i2c_write(wired_adapter.data[i].output,length,i);
		//i2c_read?
	}
}


unsigned int ogx360_i2c_isr(unsigned int cause) {
	// Do anything with i2c responses from leo boards?
	return 0;
}


void ogx360_loop()
{
	while(1)
	{
		ogx360_send_data();
		delay_us(5000);
	}
}

void ogx360_init(void)
{

	
	#if 0
		ets_printf("ogx360_init_1\n");
        /* Data */
        gpio_set_level_iram(SDA_PIN, 1);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG_IRAM[SDA_PIN], PIN_FUNC_GPIO);
        gpio_set_direction_iram(SDA_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_pull_mode_iram(SDA_PIN, GPIO_PULLUP_ONLY);
        gpio_matrix_out(SDA_PIN, I2CEXT0_SDA_OUT_IDX, false, false);
        gpio_matrix_in(SDA_PIN, I2CEXT0_SDA_IN_IDX, false);
		ets_printf("ogx360_init_2\n");
        /* Clock */
        gpio_set_level_iram(SCL_PIN, 1);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG_IRAM[SCL_PIN], PIN_FUNC_GPIO);
        gpio_set_direction_iram(SCL_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_matrix_out(SCL_PIN, I2CEXT0_SCL_OUT_IDX, false, false);
        gpio_matrix_in(SCL_PIN, I2CEXT0_SCL_OUT_IDX, false);
        gpio_set_pull_mode_iram(SCL_PIN, GPIO_PULLUP_ONLY);
		ets_printf("ogx360_init_3\n");
		periph_ll_enable_clk_clear_rst(PERIPH_I2C0_MODULE);
		ets_printf("ogx360_init_4\n");

        I2C0.int_ena.val &= I2C_LL_INTR_MASK;
        I2C0.int_clr.val = I2C_LL_INTR_MASK;
        I2C0.ctr.val = 0x3; /* Force SDA & SCL to 1 */
        I2C0.fifo_conf.fifo_addr_cfg_en = 0;
        I2C0.fifo_conf.nonfifo_en = 0;
		I2C0.ctr.ms_mode = 1;
		ets_printf("ogx360_init_5\n");
        I2C0.ctr.tx_lsb_first = I2C_DATA_MODE_MSB_FIRST;
        I2C0.ctr.rx_lsb_first = I2C_DATA_MODE_MSB_FIRST;
        I2C0.fifo_conf.tx_fifo_rst = 1;
        I2C0.fifo_conf.tx_fifo_rst = 0;
        I2C0.fifo_conf.rx_fifo_rst = 1;
        I2C0.fifo_conf.rx_fifo_rst = 0;
		ets_printf("ogx360_init_6\n");
        I2C0.fifo_conf.rx_fifo_full_thrhd = 28;
        I2C0.fifo_conf.tx_fifo_empty_thrhd = 5;
        I2C0.sda_hold.time = 40;
        I2C0.sda_sample.time = 10;
        I2C0.timeout.tout = 32000;
        I2C0.int_ena.val |= I2C_TRANS_COMPLETE_INT_ENA;
		ets_printf("ogx360_init_7\n");
		intexc_alloc_iram(ETS_I2C_EXT0_INTR_SOURCE, I2C0_INTR_NUM, ogx360_i2c_isr);
		ets_printf("ogx360_init_8\n");
		#endif
		ogx360_loop();
}
