#include "menu.h"
#include "../eeprom/eeprom.h"
#include "../lcd-routines.h"
#include "../globals.h"

#define MAXMENULINES 1

// ext. Global Function pointers
void (*RodaryTick)(int8_t);
void (*RodaryPush)();
void (*RodaryLongPush)();
void (*Blink)(uint8_t);

void (*MenuSelections[MAXMENULINES])();
void (*MenuSelectionsDraw[MAXMENULINES])(uint8_t);

char *menuTexts[] = {"Temp Thr. Pos"};

int8_t currentIndex = 0;

float fNewFloatValue = 0;
OPERATION_MODES_t newMode;
char lcd_Buffer[10];

void resetMenu()
{
    RodaryPush = NULL;
    RodaryTick = NULL;
    Blink = NULL;
}

void enableSetTempAndModeEdit()
{
    newMode = mode;
    fNewFloatValue = fSetTemp;
    RodaryTick = &editSetTemp;
    RodaryPush = &saveTemp;
    Blink = &blinkSetTemp;
}

void editSetTemp(int8_t ticks)
{
    fNewFloatValue += ticks * 0.5;

    char lcd_SetTemp[5];
    dtostrf(fNewFloatValue, 2, 1, lcd_SetTemp);

    lcd_setcursor(12, 1);
    lcd_string(lcd_SetTemp);
}

void saveTemp()
{
    blinkSetTemp(0);
    fSetTemp = fNewFloatValue;
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
    blinkMode(0);
    mode = newMode;
    resetMenu();
    writeMode(mode);
    state = WAKE_UP;
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
    currentIndex = 0;
    RodaryTick = &scrollPage;

    MenuSelections[0] = &enableTempThresholdPosEdit;
    // MenuSelectionsEdit[1] = &editTempThresholdNeg;

    MenuSelectionsDraw[0] = &drawTempThresholdPos;

    drawPage();
}

void drawPage()
{

    fNewFloatValue = fTempThrPos;
    RodaryPush = MenuSelections[currentIndex];
    (*MenuSelectionsDraw[currentIndex])(0);

    lcd_clear();
    lcd_setcursor(0, 1);
    lcd_string(menuTexts[currentIndex]);

    // draw cursor
    lcd_setcursor(0, 2);
    lcd_string(">");
}

/*void backToMenu()
{
    RodaryTick = &scrollPage;
    drawPage();
}*/

void scrollPage(int8_t ticks)
{
    // draw new page!
    currentIndex += ticks;
    /*if (currentIndex < 0)
    {
        currentIndex = 0;
    }
    else if (currentIndex > MAXMENULINES - 1)
    {
        currentIndex = MAXMENULINES - 1;
    }*/
    drawPage();
}

void enableTempThresholdPosEdit()
{
    fNewFloatValue = fTempThrPos;
    RodaryTick = &editTempThresholdPos;
    RodaryPush = &saveTempThresholdPos;
    Blink = &drawTempThresholdPos;
}
void editTempThresholdPos(int8_t ticks)
{
    fNewFloatValue += ticks * 0.5;
    drawTempThresholdPos(0);
}
void drawTempThresholdPos(uint8_t toggle)
{
    lcd_setcursor(5, 2);
    if (toggle)
    {
        lcd_string("      ");
    }
    else
    {
        dtostrf(fNewFloatValue, 2, 1, lcd_Buffer);
        lcd_string(lcd_Buffer);
    }
}
void saveTempThresholdPos()
{
    fTempThrPos = fNewFloatValue;
    //Blink = NULL;
    //backToMenu();
}