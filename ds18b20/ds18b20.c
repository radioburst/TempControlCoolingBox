#include "ds18b20.h"
#include "1wire.h"

#include <avr/io.h>

void start_meas(void)
{
  if (W1_IN & 1 << W1_PIN)
  {
    w1_command(CONVERT_T, NULL);
    W1_OUT |= 1 << W1_PIN;
    W1_DDR |= 1 << W1_PIN; // parasite power on
  }
}

uint8_t read_meas(float *ftemp)
{
  uchar id[8], diff;
  int16_t temp = 0;

  for (diff = SEARCH_FIRST; diff != LAST_DEVICE;)
  {
    diff = w1_rom_search(diff, id);

    if (diff == PRESENCE_ERR)
    {
      return 1;
    }
    if (diff == DATA_ERR)
    {
      return 1;
    }
    if (id[0] == 0x28 || id[0] == 0x10)
    {                            // temperature sensor
      w1_byte_wr(READ);          // read command
      temp = w1_byte_rd();       // low byte
      temp |= w1_byte_rd() << 8; // high byte

      *ftemp = (float)temp / 16.0;
    }
    else
    {
      return 1;
    }
  }
  return 0;
}
