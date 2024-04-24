#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>


#ifndef W1_PIN
#define W1_PIN	PB6
#define W1_IN	PINB
#define W1_OUT	PORTB
#define W1_DDR	DDRB
#endif

#define MATCH_ROM	0x55
#define SKIP_ROM	0xCC
#define	SEARCH_ROM	0xF0

#define CONVERT_T	0x44		// DS1820 commands
#define READ		0xBE
#define WRITE		0x4E
#define EE_WRITE	0x48
#define EE_RECALL	0xB8

#define	SEARCH_FIRST	0xFF		// start new search
#define	PRESENCE_ERR	0xFF
#define	DATA_ERR	0xFE
#define LAST_DEVICE	0x00

#define NULL 0		


unsigned char w1_reset(void)
{
  unsigned char err;

  W1_OUT &= ~(1<<W1_PIN);
  W1_DDR |= 1<<W1_PIN;
  _delay_us(480);			// 480 us
  cli();
  W1_DDR &= ~(1<<W1_PIN);
  _delay_us(66);
  err = W1_IN & (1<<W1_PIN);			// no presence detect
  sei();
  _delay_us( 480 - 66 );
  if( (W1_IN & (1<<W1_PIN)) == 0 )		// short circuit
    err = 1;
  return err;
}


unsigned char w1_bit_io( unsigned char b )
{
  cli();
  W1_DDR |= 1<<W1_PIN;
  _delay_us( 1 );
  if( b )
    W1_DDR &= ~(1<<W1_PIN);
  _delay_us( 15 - 1 );
  if( (W1_IN & (1<<W1_PIN)) == 0 )
    b = 0;
  _delay_us( 60 - 15 );
  W1_DDR &= ~(1<<W1_PIN);
  sei();
  return b;
}


unsigned char w1_byte_wr( unsigned char b )
{
  unsigned char i = 8, j;
  do{
    j = w1_bit_io( b & 1 );
    b >>= 1;
    if( j )
      b |= 0x80;
  }while( --i );
  return b;
}


unsigned char w1_byte_rd( void )
{
  return w1_byte_wr( 0xFF );
}




void w1_command( unsigned char command, unsigned char *id )
{
  unsigned char i;
  w1_reset();
  if( id ){
    w1_byte_wr( MATCH_ROM );			// to a single device
    i = 8;
    do{
      w1_byte_wr( *id );
      id++;
    }while( --i );
  }else{
    w1_byte_wr( SKIP_ROM );			// to all devices
  }
  w1_byte_wr( command );
}

void start_meas( void ){
  if( W1_IN & 1<< W1_PIN ){
    w1_command( CONVERT_T, NULL );
    W1_OUT |= 1<< W1_PIN;
    W1_DDR |= 1<< W1_PIN;			// parasite power on

  }else{
  	//DDRC|=(1<<DDC7);
	//PORTC|=(1<<PC7);
  }
}
