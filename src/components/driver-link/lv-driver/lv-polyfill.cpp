
#pragma GCC optimize ("O3")

#include "lv-engine/engine.h"
#include "lv-driver/display.h"

#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "rom/ets_sys.h"

#define ODROID_GAMEPAD_IO_X ADC1_CHANNEL_6
#define ODROID_GAMEPAD_IO_Y ADC1_CHANNEL_7
#define ODROID_GAMEPAD_IO_SELECT GPIO_NUM_27
#define ODROID_GAMEPAD_IO_START GPIO_NUM_39
#define ODROID_GAMEPAD_IO_A GPIO_NUM_32
#define ODROID_GAMEPAD_IO_B GPIO_NUM_33
#define ODROID_GAMEPAD_IO_MENU GPIO_NUM_13
#define ODROID_GAMEPAD_IO_VOLUME GPIO_NUM_0

static lv::half palette[32] = {

	0b0000000000000000,
	0b0000011000100001,
	0b0100011101000001,
	0b1100011001100001,
	0b1010011110001010,
	0b1000010011011011,
	0b0000110011011101,
	0b0001001111101110,
	0b1000011011111111,
	0b0010101010011111,
	0b1110011001101101,
	0b1010110100110100,
	0b0100010101001011,
	0b0100010001010010,
	0b1110011100110001,
	0b1110111000111001,
	0b0001000000110011,
	0b0111110001011011,
	0b1101000000110010,
	0b0111110001011110,
	0b1101111111001110,
	0b1111111111111111,
	0b0111011010011101,
	0b1111000010000011,
	0b0100110101101011,
	0b1010101001011010,
	0b0001000101110010,
	0b1000011010101001,
	0b1010110011011010,
	0b1101011111010011,
	0b1010100110001100,
	0b0110011010001011
};

extern "C" {

    void lvDriver_DrawHLine(const lv::half line, const lv::octet (&stream)[lvk_display_w]){

    	register lv::word pixelCursor = 0;
		
    	do {
			#if kDisplayScale == 1
			 	*(hLinePixels + (line%kBufferingLines * lvk_display_w) + pixelCursor) = palette[stream[pixelCursor]];
			#elif kDisplayScale == 2
				*(hLinePixels + (((line * 2)+0)%kBufferingLines * kDisplayPyshicalWidth) + (pixelCursor*2 + 0)) = palette[stream[pixelCursor]];
				*(hLinePixels + (((line * 2)+0)%kBufferingLines * kDisplayPyshicalWidth) + (pixelCursor*2 + 1)) = palette[stream[pixelCursor]];
				*(hLinePixels + (((line * 2)+1)%kBufferingLines * kDisplayPyshicalWidth) + (pixelCursor*2 + 0)) = palette[stream[pixelCursor]];
				*(hLinePixels + (((line * 2)+1)%kBufferingLines * kDisplayPyshicalWidth) + (pixelCursor*2 + 1)) = palette[stream[pixelCursor]];
			#endif
    	} while (pixelCursor++ < lvk_display_w);

		if(line%(kBufferingLines/kDisplayScale) == (kBufferingLines/kDisplayScale) - 1) {
			ili9341_send_continue_line((uint16_t*) hLinePixels, kDisplayPyshicalWidth, kBufferingLines);
		}

    }

    extern lv::octet lvDriver_CurrentFPS(void){
        return measuredFPS;
    }

	lv::GamePad lvDriver_GamePadState(lv::half player) {

		lv::GamePad p = {0};

		// directional keys will be using a voltage divider + analog port

		int x_series = 0;
		int y_series = 0;
		int series_size = 3;

		for(int i = 0; i < series_size; i++) {
			x_series += adc1_get_raw(ODROID_GAMEPAD_IO_X);
			y_series += adc1_get_raw(ODROID_GAMEPAD_IO_Y);
			ets_delay_us(6);
		}

		int xLevel 	= x_series/series_size;
    	int yLevel 	= y_series/series_size;

    	if (xLevel > 4000){
        	p.left 	= 0;
        	p.rigth = 0;
		} else if (xLevel > 2000){
        	p.left 	= 0;
        	p.rigth = 1;
    	} else {
        	p.left 	= 1;
        	p.rigth = 0;
		}

		if (yLevel > 4000){
        	p.up 	= 0;
        	p.down 	= 0;
		} else if (yLevel > 2000){
        	p.up 	= 0;
        	p.down 	= 1;
    	} else {
        	p.up 	= 1;
        	p.down 	= 0;
		}

		// all the others are digital ports

		p.select 	= !(gpio_get_level(ODROID_GAMEPAD_IO_SELECT));
    	p.start 	= !(gpio_get_level(ODROID_GAMEPAD_IO_START));

		p.a 		= !gpio_get_level(ODROID_GAMEPAD_IO_A);
		p.b 		= !gpio_get_level(ODROID_GAMEPAD_IO_B);
		p.x 		= 0; // dont have
		p.y 		= 0; // dont have

		return p;
	}
}