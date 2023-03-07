#include <esp32/rom/ets_sys.h>
#include "adapter/adapter.h"
#include "gameport_spi.h"
#include "adapter/wired/gameport.h"
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "driver/i2c.h"



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


#define I2C_MASTER_SCL_IO 21               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 22               /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 400000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define SLAVE_ADDRESS 1 

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x0                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

int i2c_master_port = 0;

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

    //Initialize I2C
        i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err == ESP_OK) {
        i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    }
    else
    {
        ets_printf("i2c init failed\n");
    }


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

static esp_err_t gameport_i2c_master_send(uint8_t message[], int len)
{
     
    esp_err_t ret; 
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SLAVE_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, message, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


bool gameport_initialized = false;

uint8_t test[] = "kHello World";
void gameport_process(uint8_t player)
{
    if (!gameport_initialized)
    {
        gameport_initialize();
        gameport_initialized = true;
    }
	
	struct gameport_data *data = (struct gameport_data*)&wired_adapter.data[player].output;
    if (data->magic == 'g')
    {
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
    else if(data->magic == 'k')
    {
        struct gameport_kb_data *kb_data = (struct gameport_kb_data*) &wired_adapter.data[player].output;
        gameport_i2c_master_send(kb_data,sizeof(struct gameport_kb_data));
    } 
    

}
