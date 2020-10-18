/*
//
//  lvndr display driver for the ili9341 LCD TFT display
//  based on:
//  https://github.com/OtherCrashOverride/odroid-go-firmware
//  by OtherCrashOverride
*/

#pragma once

#include "stdint.h"

#define kBufferingLines 60

extern unsigned short* hLinePixels;
extern unsigned short measuredFPS;

void ili9341_init();
void ili9341_clear(uint16_t color);

void ili9341_begin_drawing();
void ili9341_send_continue_line(uint16_t *line, int width, int lineCount);
void ili9341_end_drawing();

void ili9341_backlight_deinit();