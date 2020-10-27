#pragma GCC optimize ("O3")

#include "lv-driver/display.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"

#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"

#include "lv-game/lvk.h"


const gpio_num_t SPI_PIN_NUM_MISO = GPIO_NUM_19;
const gpio_num_t SPI_PIN_NUM_MOSI = GPIO_NUM_23;
const gpio_num_t SPI_PIN_NUM_CLK  = GPIO_NUM_18;

const gpio_num_t LCD_PIN_NUM_CS   = GPIO_NUM_5;
const gpio_num_t LCD_PIN_NUM_DC   = GPIO_NUM_21;
const gpio_num_t LCD_PIN_NUM_BCKL = GPIO_NUM_14;

const int LCD_BACKLIGHT_ON_VALUE = 1;
const int LCD_SPI_CLOCK_RATE = SPI_MASTER_FREQ_80M;

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08

static spi_transaction_t trans[8];
static spi_device_handle_t spi;
static TaskHandle_t xTaskToNotify = NULL;

#define LINE_COUNT (kBufferingLines)
const int DUTY_MAX = 0x1fff;

unsigned short* hLinePixels;
unsigned short measuredFPS;

typedef struct {
    uint8_t cmd;
    uint8_t data[128];
    uint8_t databytes;
} ili_init_cmd_t;

#define TFT_CMD_SWRESET	0x01
#define TFT_CMD_SLEEP 0x10
#define TFT_CMD_DISPLAY_OFF 0x28

static const ili_init_cmd_t ili_sleep_cmds[] = {
    {TFT_CMD_SWRESET, {0}, 0x80},
    {TFT_CMD_DISPLAY_OFF, {0}, 0x80},
    {TFT_CMD_SLEEP, {0}, 0x80},
    {0, {0}, 0xff}
};

static const ili_init_cmd_t ili_init_cmds[] = {
    
    {TFT_CMD_SWRESET, {0}, 0x80},
    {0xCF, {0x00, 0xc3, 0x30}, 3},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    {0xE8, {0x85, 0x00, 0x78}, 3},
    {0xCB, {0x39, 0x2c, 0x00, 0x34, 0x02}, 5},
    {0xF7, {0x20}, 1},
    {0xEA, {0x00, 0x00}, 2},
    {0xC0, {0x1B}, 1}, //Power control   //VRH[5:0]
    {0xC1, {0x12}, 1}, //Power control   //SAP[2:0];BT[3:0]
    {0xC5, {0x32, 0x3C}, 2}, //VCM control
    {0xC7, {0x91}, 1}, //VCM control2
    {0x36, {(MADCTL_MV | MADCTL_MY | TFT_RGB_BGR)}, 1},    // Memory Access Control
    {0x3A, {0x55}, 1},
    {0xB1, {0x00, 0x1F}, 2}, // Frame Rate Control (1B=70, 1F=61, 10=119)
    {0xB6, {0x0A, 0xA2}, 2}, // Display Function Control
    {0xF6, {0x00, 0x30}, 2},
    {0xF2, {0x00}, 1}, // 3Gamma Function Disable
    {0x26, {0x01}, 1}, //Gamma curve selected

    //Set Gamma
    {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
    {0XE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},

    // LUT
    {0x2d, {0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f,
            0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d, 0x3f,
            0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
            0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
            0x1d, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26, 0x27, 0x28, 0x29, 0x2a,
            0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x12, 0x14, 0x16, 0x18, 0x1a,
            0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38}, 128},

    {0x11, {0}, 0x80},    //Exit Sleep
    {0x29, {0}, 0x80},    //Display on
    {0, {0}, 0xff}
};

static void ili_cmd(spi_device_handle_t spi, const uint8_t cmd){

    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 

    t.length = 8;
    t.tx_buffer = &cmd;
    t.user=(void*) 0;

    assert(
        spi_device_polling_transmit(spi, &t) == ESP_OK
    );
}

static void ili_data(spi_device_handle_t spi, const uint8_t *data, int len){

    if (len == 0) return;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = len*8;
    t.tx_buffer = data;
    t.user = (void*) 1;
    
    assert(
        spi_device_polling_transmit(spi, &t) == ESP_OK
    );
}

static void ili_spi_pre_transfer_callback(spi_transaction_t *t){
    int dc = (int)t->user;
    gpio_set_level(LCD_PIN_NUM_DC, dc);
}

static void ili_spi_post_transfer_callback(spi_transaction_t *t){
    if(xTaskToNotify && t == &trans[7]){
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR( xTaskToNotify, &xHigherPriorityTaskWoken );
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

static void ili_init(){

    int cmd = 0;

    gpio_set_direction(LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    while (ili_init_cmds[cmd].databytes!=0xff) {
        
        ili_cmd(spi, ili_init_cmds[cmd].cmd);
        ili_data(spi, ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x7f);

        if (ili_init_cmds[cmd].databytes & 0x80) vTaskDelay(100 / portTICK_RATE_MS);
        cmd++;

    }
    
    const size_t linesSize = (lvk_display_w * kBufferingLines * 2);
    hLinePixels = (unsigned short*) heap_caps_malloc(linesSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
}

static void send_reset_drawing(int left, int top, int width, int height){

    trans[0].tx_data[0] = 0x2A; //Column Address Set
    trans[1].tx_data[0] = (left) >> 8; //Start Col High
    trans[1].tx_data[1] = (left) & 0xff; //Start Col Low
    trans[1].tx_data[2] = (left + width - 1) >> 8; //End Col High
    trans[1].tx_data[3] = (left + width - 1) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B; //Page address set
    trans[3].tx_data[0] = top >> 8; //Start page high
    trans[3].tx_data[1] = top & 0xff; //start page low
    trans[3].tx_data[2] = (top + height - 1)>>8; //end page high
    trans[3].tx_data[3] = (top + height - 1)&0xff; //end page low
    trans[4].tx_data[0] = 0x2C; //memory write

    for (int x = 0; x < 5; x++) {
        assert(
            spi_device_polling_transmit(spi, &trans[x]) == ESP_OK
        );
    }
}

void ili9341_begin_drawing() {
    send_reset_drawing(0, 0, lvk_display_w, lvk_display_h);
}

void ili9341_end_drawing(){
    
}

void ili9341_send_continue_line(uint16_t *line, int width, int lineCount){

    trans[6].tx_data[0] = 0x3C; //memory write continue
    trans[6].length = 8; //Data length, in bits
    trans[6].flags = SPI_TRANS_USE_TXDATA;
    trans[6].rxlength = 0;

    trans[7].tx_buffer = line; // finally send the line data
    trans[7].length = width * lineCount * 2 * 8; //Data length, in bits
    trans[7].flags = 0; //undo SPI_TRANS_USE_TXDATA flag
    trans[7].rxlength = 0;

    for (int x = 6; x < 8; x++) {
        assert(
            spi_device_polling_transmit(spi, &trans[x]) == ESP_OK
        );
    }
}

static void backlight_init() {

    ledc_timer_config_t ledc_timer;
    memset(&ledc_timer, 0, sizeof(ledc_timer));

    ledc_timer.bit_num = LEDC_TIMER_13_BIT; //set timer counter bit number
    ledc_timer.freq_hz = 5000; //set frequency of pwm
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE; //timer mode,
    ledc_timer.timer_num = LEDC_TIMER_0; //timer index
    ledc_timer_config(&ledc_timer);

    //set the configuration
    ledc_channel_config_t ledc_channel;
    memset(&ledc_channel, 0, sizeof(ledc_channel));

    // set LEDC channel 0
    ledc_channel.channel = LEDC_CHANNEL_0;
    // set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
    ledc_channel.duty = (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX;
    // GPIO number
    ledc_channel.gpio_num = LCD_PIN_NUM_BCKL;
    // GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
    ledc_channel.intr_type = LEDC_INTR_FADE_END;
    // set LEDC mode, from ledc_mode_t
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    // set LEDC timer source, if different channel use one timer,
    // the frequency and bit_num of these channels should be the same
    ledc_channel.timer_sel = LEDC_TIMER_0;

    ledc_channel_config(&ledc_channel);

    //initialize fade service.
    ledc_fade_func_install(0);

    // duty range is 0 ~ ((2**bit_num)-1)
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? DUTY_MAX : 0, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);

}

void ili9341_backlight_deinit(){
    ledc_fade_func_uninstall();
    esp_err_t err = ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    if (err != ESP_OK) printf("%s: ledc_stop failed.\n", __func__);
}

void ili9341_clear(uint16_t color){ 
    send_reset_drawing(0, 0, 320, 240);
    for (int i = 0; i < 320; ++i) *(hLinePixels + i ) = color;
    for (int y = 0; y < 240; ++y) ili9341_send_continue_line((uint16_t*) hLinePixels, 320, 1);
}

void ili9341_init(){ 
    
    // Initialize transactions
    for (int x = 0; x < 8; x++) {
        
        memset(&trans[x], 0, sizeof(spi_transaction_t));

        //Even transfers are commands
        if ((x&1) == 0) {
            trans[x].length = 8;
            trans[x].user = (void*)0;

        //Odd transfers are data
        } else {
            trans[x].length = 8*4;
            trans[x].user = (void*)1;
        }

        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }

    // Initialize SPI
    spi_bus_config_t buscfg;
	memset(&buscfg, 0, sizeof(buscfg));

    buscfg.miso_io_num = SPI_PIN_NUM_MISO;
    buscfg.mosi_io_num = SPI_PIN_NUM_MOSI;
    buscfg.sclk_io_num = SPI_PIN_NUM_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = kBufferingLines * 320 * 2 + 8;

    spi_device_interface_config_t devcfg;
	memset(&devcfg, 0, sizeof(devcfg));

    devcfg.clock_speed_hz = LCD_SPI_CLOCK_RATE;
    devcfg.mode = 0; //SPI mode 0
    devcfg.spics_io_num = LCD_PIN_NUM_CS;
    devcfg.queue_size = 7; // We want to be able to queue 7 transactions at a time
    devcfg.pre_cb = ili_spi_pre_transfer_callback;
    devcfg.post_cb = ili_spi_post_transfer_callback;
    devcfg.flags = SPI_DEVICE_NO_DUMMY;

    //Initialize the SPI bus
    assert( spi_bus_initialize(HSPI_HOST, &buscfg, 1) == ESP_OK);

    //Attach the LCD to the SPI bus
    assert(spi_bus_add_device(HSPI_HOST, &devcfg, &spi) == ESP_OK);

    // lock bus only for LCD
    spi_device_acquire_bus(spi, portMAX_DELAY);

    //Initialize the LCD
	printf("[Odroid LCD :: lvndr driver] calling ili_init\n");
    ili_init();

	printf("[Odroid LCD :: lvndr driver] calling backlight_init\n");
    backlight_init();

    printf("[Odroid LCD :: lvndr driver] Initialized (%d Hz).\n", LCD_SPI_CLOCK_RATE);

}