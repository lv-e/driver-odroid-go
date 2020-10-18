
#pragma GCC optimize ("O3")

#include "lv-engine/engine.h"
#include "lv-driver/display.h"

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

    void lvDriver_DrawHLine(lv::half line, lv::octet (&stream)[lvk_display_w]){

    	register lv::word pixelCursor = 0;

    	do {
        	*(hLinePixels + (line%kBufferingLines * lvk_display_w) + pixelCursor) = palette[stream[pixelCursor]];
    	} while (pixelCursor++ < lvk_display_w);

		if(line%kBufferingLines == kBufferingLines - 1) {
			ili9341_send_continue_line((uint16_t*) hLinePixels, 320, kBufferingLines);
		}

    }

    extern lv::octet lvDriver_CurrentFPS(void){
        return measuredFPS;
    }

}