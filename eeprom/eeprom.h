#ifndef _eeprom_h_
#define _eeprom_h_

#include <avr/eeprom.h>
#include "../globals.h"

#define EEPROM_ADDR_SET_TEMP 32 // 3 Bytes
#define EEPROM_ADDR_MODE 35     // 1 Byte

float readSetTemp();
void writeSetTemp(float temp);

OPERATION_MODES_t readMode();
void writeMode(OPERATION_MODES_t mode);

#endif