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

#ifndef DS1820_H
#define DS1820_H

unsigned char w1_reset(void);

unsigned char w1_bit_io( unsigned char b );

unsigned char w1_byte_wr( unsigned char b );

unsigned char w1_byte_rd( void );

void w1_command( unsigned char command, unsigned char *id );

void start_meas( void );

#endif //DS1820_H
