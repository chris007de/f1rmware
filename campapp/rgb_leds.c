#include <rad1olib/light_ws2812_cortex.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/config.h>

#define MAX_LED_FRAMES 50

unsigned char leds[3*8*MAX_LED_FRAMES];
unsigned char leds_off[3*8];
unsigned int frames = 0;

void rgbLedsInit(void) {
	char filename[] = "rgb_leds.hex";
	int size = getFileSize(filename);
	if(size > 0) {
		if(size >= 3*8*MAX_LED_FRAMES)
			size = 3*8*MAX_LED_FRAMES;
		readFile(filename, (char*)leds, size);
		frames = (size-1)/(3*8);
	}

	// Todo clear leds_off memory
}

void rgbLedsTick(void) {
	static unsigned int ledctr = 0;
	static unsigned int ctr = 0;
	ctr++;

	if(GLOBAL(ws2812_active))
	{
		if(frames > 0) {
			// LED delay is in leds[0:1]
			if(ctr >= ((leds[0]<<8) + leds[1])){
				ws2812_sendarray(&leds[ledctr*3*8+2], 3*8);
				ledctr++;
				if(ledctr >= frames)
					ledctr = 0;

				ctr = 0;
			}
		}
	}
	else
	{
		ws2812_sendarray(leds_off, sizeof(leds_off));
	}
	return;
}
