#ifndef _fan_h_
#define _fan_h_

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/*
Use Timer1 the Atmega8 to controll two FANs via PWM.
Outputs are on the timer pins PB1 & PB2
*/

typedef enum FANS_t
{
    FAN1 = 1,
    FAN2
} FANS_t;

void fan_init()
{
    // Set PB1 (OC1A) and PB2 (OC1B) as output
    DDRB |= (1 << PB1) | (1 << PB2);

    // Set Timer1 to Fast PWM mode with ICR1 as TOP
    TCCR1A = (1 << WGM11) | (1 << COM1A1) | (1 << COM1B1);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // No prescaler

    // Set ICR1 to define the PWM frequency (160kHz)
    ICR1H = (100 >> 8); 
    ICR1L = 100;

    stop_fan(FAN1);
    stop_fan(FAN2);
}

void stop_fan(FANS_t fan)
{
    if (fan == FAN1)
    {
        OCR1AH = 0;
        OCR1AL = 0;
    }
    else if (fan == FAN2)
    {
        OCR1BH = 0;
        OCR1BL = 0;
    }
}

void set_fan_pwn(FANS_t fan, float value)
{
    if (fan == FAN1)
    {
        uint16_t ocr_value = (uint16_t)((value / 100) * (ICR1 + 1));
        OCR1AH = (ocr_value >> 8);
        OCR1AL = ocr_value;
    }
    else if (fan == FAN2)
    {
        uint16_t ocr_value = (uint16_t)((value / 100) * (ICR1 + 1));
        OCR1BH = (ocr_value >> 8);
        OCR1BL = ocr_value;
    }
}

#endif