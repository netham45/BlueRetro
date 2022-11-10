#include <esp32/rom/ets_sys.h>
#include "adapter/adapter.h"
#include "gameport_spi.h"
#include "adapter/wired/gameport.h"
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define GAMEPORT_PRESSED 0
#define GAMEPORT_RELEASED 1

#define GAMEPORT_PIN_MISO 19
#define GAMEPORT_PIN_MOSI 23
#define GAMEPORT_PIN_CLK  18
#define GAMEPORT_PIN_CS   05

#define GAMEPORT_POT_CHANNEL_X1 1
#define GAMEPORT_POT_CHANNEL_Y1 3
#define GAMEPORT_POT_CHANNEL_X2 0
#define GAMEPORT_POT_CHANNEL_Y2 2

// Address 0 - 
// Address 1 - 
// Address 2 - 
// Address 3 - 


#define GAMEPORT_BUTTON_PRESSED 0
#define GAMEPORT_BUTTON_RELEASED 1

#define GAMEPORT_BUTTON_PIN_1 12
#define GAMEPORT_BUTTON_PIN_2 14
#define GAMEPORT_BUTTON_PIN_3 27
#define GAMEPORT_BUTTON_PIN_4 13

const uint8_t GAMEPORT_BUTTON_PINS[] = {GAMEPORT_BUTTON_PIN_1, GAMEPORT_BUTTON_PIN_2, GAMEPORT_BUTTON_PIN_3, GAMEPORT_BUTTON_PIN_4};

const uint32_t GPIO_OUTPUT_PIN_SEL = (1 << GAMEPORT_BUTTON_PIN_1) |
									 (1 << GAMEPORT_BUTTON_PIN_2) |
									 (1 << GAMEPORT_BUTTON_PIN_3) |
									 (1 << GAMEPORT_BUTTON_PIN_4) |
									 (1 << GAMEPORT_PIN_CS);
									 
const uint8_t AXIS_MAP[] = { GAMEPORT_POT_CHANNEL_X1, GAMEPORT_POT_CHANNEL_Y1, GAMEPORT_POT_CHANNEL_X2, GAMEPORT_POT_CHANNEL_Y2 } ;

spi_device_handle_t gameport_spi;
struct gameport_spi_data {
	uint8_t address;
	uint8_t value;
};

void gameport_spi_transmit(const uint8_t *data, int len) 
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=16;
    t.tx_buffer=data;
	gpio_set_level(GAMEPORT_PIN_CS, 0);
    ret=spi_device_transmit(gameport_spi, &t);  //Transmit!
	gpio_set_level(GAMEPORT_PIN_CS, 1);
    assert(ret==ESP_OK);            //Should have had no issues.
}

void gameport_initialize(void) {
	//Initialize SPI
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=GAMEPORT_PIN_MISO,
        .mosi_io_num=GAMEPORT_PIN_MOSI,
        .sclk_io_num=GAMEPORT_PIN_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=10*1000*1000,               //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        //.spics_io_num=GAMEPORT_PIN_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &gameport_spi);

	//Initialize GPIO
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
	gpio_set_level(GAMEPORT_PIN_CS, 1);
}

bool gameport_initialized = false;

void gameport_process()
{
    if (!gameport_initialized)
    {
        gameport_initialize();
        gameport_initialized = true;
    }
	
	struct gameport_data *data = (struct gameport_data*)&wired_adapter.data[0].output;
	for (int i=0;i<4;i++)
	{
		gpio_set_level(GAMEPORT_BUTTON_PINS[i], data->buttons[i] ? GAMEPORT_PRESSED : GAMEPORT_RELEASED);
	}
	
	struct gameport_spi_data spi_data;
	for (int i=0;i<4;i++)
	{
		spi_data.address = AXIS_MAP[i];
		if (AXIS_MAP[i] == GAMEPORT_POT_CHANNEL_X1 || AXIS_MAP[i] == GAMEPORT_POT_CHANNEL_X2)
		{
			spi_data.value = ((data->axis[i] - 127) * -1) + 128; //TODO: Move to gameport_from_generic
		}
		else
		{
			spi_data.value = data->axis[i];
		}
		gameport_spi_transmit((const uint8_t*)&spi_data,2);
	}
}