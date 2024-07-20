#ifndef _rotary_encoder_h_
#define _ds18b20_h_

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define PHASE_A (PIND & 1 << PD3)
#define PHASE_B (PIND & 1 << PD4)

volatile int8_t enc_delta = 0;

void encode_init()
{
    // ext INT0
    // rising edge on INT0
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    // global interrupt flag for INT0
    GICR |= (1 << INT0);
    // internal pull up for INT0 pin
    PORTD |= (1 << DDD2);

    DDRD &= ~(1 << DDD4); // Rodary A
    DDRD &= ~(1 << DDD3); // Rodary B
    DDRD &= ~(1 << DDD2); // Rodary Push
    PORTD |= (1 << PD4);  // int. pull up
    PORTD |= (1 << PD3);  // int. pull up
    PORTD |= (1 << PD2);  // int. pull up

    TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12); // CTC, XTAL / 64
    OCR1A = (uint8_t)(F_CPU / 64 * 1e-3 - 0.5);        // 2ms
}

int8_t encode_read4() // read four step encoders
{
    int8_t val;

    cli();
    val = enc_delta;
    enc_delta &= 3;
    sei();
    return val >> 2;
}

ISR(TIMER1_COMPA_vect) // 2ms for manual movement
{
    static int8_t last;
    int8_t newVal, diff;

    newVal = 0;
    if (PHASE_A)
        newVal = 3;
    if (PHASE_B)
        newVal ^= 1;      // convert gray to binary
    diff = last - newVal; // difference last - newVal
    if (diff & 1)
    {                                // bit 0 = value (1)
        last = newVal;               // store newVal as next last
        enc_delta += (diff & 2) - 1; // bit 1 = direction (+/-)
    }
}

#endif