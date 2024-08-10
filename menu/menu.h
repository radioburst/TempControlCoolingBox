#ifndef _menu_h_
#define _menu_h_

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

extern void (*RodaryTick)(int8_t);
extern void (*RodaryPush)();
extern void (*RodaryLongPush)();
extern void (*Blink)(uint8_t);

void resetMenu();

void enableSetTempAndModeEdit();
// set temp
void editSetTemp(int8_t ticks);
void saveTemp();
void blinkSetTemp(uint8_t toggle);
// set mode
void editMode(int8_t ticks);
void saveMode();
void blinkMode(uint8_t toggle);

void drawMenu();
void drawPage();
//void backToMenu();
void scrollPage(int8_t ticks);

void enableTempThresholdPosEdit();
void editTempThresholdPos(int8_t ticks);
void drawTempThresholdPos(uint8_t toggle);
void saveTempThresholdPos();

#endif