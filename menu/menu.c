#include "menu.h"
#include "../eeprom/eeprom.h"
#include "../lcd-routines.h"
#include "../globals.h"

// ext. Global Function pointers
void (*RodaryTick)(int8_t);
void (*RodaryPush)();
void (*RodaryLongPush)();
void (*Blink)(uint8_t);

uint8_t currentPage = 0;

float fNewSetTemp = 0;
OPERATION_MODES_t newMode;

void resetMenu()
{
    RodaryPush = NULL;
    RodaryTick = NULL;
    Blink = NULL;
}

void enableSetTempAndModeEdit()
{
    newMode = mode;
    fNewSetTemp = fSetTemp;
    RodaryTick = &editSetTemp;
    RodaryPush = &saveTemp;
    Blink = &blinkSetTemp;
}

void editSetTemp(int8_t ticks)
{
    fNewSetTemp += ticks * 0.5;

    char lcd_SetTemp[5];
    dtostrf(fNewSetTemp, 2, 1, lcd_SetTemp);

    lcd_setcursor(12, 1);
    lcd_string(lcd_SetTemp);
}

void saveTemp()
{
    fSetTemp = fNewSetTemp;
    RodaryPush = &saveMode;
    RodaryTick = &editMode;
    Blink = &blinkMode;
    writeSetTemp(fSetTemp);
}

void blinkSetTemp(uint8_t toggle)
{
    if (toggle)
    {

        lcd_setcursor(12, 1);
        lcd_string("    ");
    }
    else
    {
        editSetTemp(0);
    }
}

void editMode(int8_t ticks)
{
    newMode += ticks;

    if (newMode > AC)
    {
        newMode = GAS;
    }
    else if (newMode < GAS)
    {
        newMode = AC;
    }

    lcd_setcursor(2, 2);
    if (newMode == GAS)
        lcd_string("GA");
    else if (newMode == DC)
        lcd_string("DC");
    else
        lcd_string("AC");
}

void saveMode()
{
    mode = newMode;
    resetMenu();
    writeMode(mode);
    state = IDLE;
}

void blinkMode(uint8_t toggle)
{
    if (toggle)
    {

        lcd_setcursor(2, 2);
        lcd_string("  ");
    }
    else
    {
        editMode(0);
    }
}

void drawMenu()
{
    lcd_clear();
    lcd_string("MENU");
}
