
#include "ds1820.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

unsigned char w1_reset(void)
{
  unsigned char err;

  W1_OUT &= ~(1 << W1_PIN);
  W1_DDR |= 1 << W1_PIN;
  _delay_us(480); // 480 us
  cli();
  W1_DDR &= ~(1 << W1_PIN);
  _delay_us(66);
  err = W1_IN & (1 << W1_PIN); // no presence detect
  sei();
  _delay_us(480 - 66);
  if ((W1_IN & (1 << W1_PIN)) == 0) // short circuit
    err = 1;
  return err;
}

unsigned char w1_bit_io(unsigned char b)
{
  cli();
  W1_DDR |= 1 << W1_PIN;
  _delay_us(1);
  if (b)
    W1_DDR &= ~(1 << W1_PIN);
  _delay_us(15 - 1);
  if ((W1_IN & (1 << W1_PIN)) == 0)
    b = 0;
  _delay_us(60 - 15);
  W1_DDR &= ~(1 << W1_PIN);
  sei();
  return b;
}

unsigned char w1_byte_wr(unsigned char b)
{
  unsigned char i = 8, j;
  do
  {
    j = w1_bit_io(b & 1);
    b >>= 1;
    if (j)
      b |= 0x80;
  } while (--i);
  return b;
}

unsigned char w1_byte_rd(void)
{
  return w1_byte_wr(0xFF);
}

void w1_command(unsigned char command, unsigned char *id)
{
  unsigned char i;
  w1_reset();
  if (id)
  {
    w1_byte_wr(MATCH_ROM); // to a single device
    i = 8;
    do
    {
      w1_byte_wr(*id);
      id++;
    } while (--i);
  }
  else
  {
    w1_byte_wr(SKIP_ROM); // to all devices
  }
  w1_byte_wr(command);
}

void start_meas(void)
{
  if (W1_IN & 1 << W1_PIN)
  {
    w1_command(CONVERT_T, NULL);
    W1_OUT |= 1 << W1_PIN;
    W1_DDR |= 1 << W1_PIN; // parasite power on
  }
}
