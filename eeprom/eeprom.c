#include "eeprom.h"

float readSetTemp()
{
    float tempSet = eeprom_read_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 1);
    if (eeprom_read_byte((uint8_t *)EEPROM_ADDR_SET_TEMP) == 1)
    {
        tempSet *= (-1);
    }
    if (eeprom_read_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 2) == 1)
    {
        if (tempSet < 0)
        {
            tempSet -= 0.5;
        }
        else
        {
            tempSet += 0.5;
        }
    }
    return tempSet;
}

void writeSetTemp(float temp)
{
    if (temp < 0.0) // minus
    {
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP, (uint8_t)1);
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 1, (uint8_t)(temp * (-1)));
    }
    else
    {
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP, (uint8_t)0);
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 1, (uint8_t)temp);
    }

    if (temp == (int)temp) // nachkommerstelle
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 2, (uint8_t)0);
    else
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_SET_TEMP + 2, (uint8_t)1);
}

OPERATION_MODES_t readMode()
{
    uint8_t raw = eeprom_read_byte((uint8_t *)EEPROM_ADDR_MODE);
    if (raw > AC)
    {
        return AC;
    }
    return raw;
}

void writeMode(OPERATION_MODES_t mode)
{
    eeprom_write_byte((uint8_t *)EEPROM_ADDR_MODE, (uint8_t)mode);
}