#include <avr/io.h>
#include "lcd-routines.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#define W1_PIN	PD6
#define W1_IN	PIND
#define W1_OUT	PORTD
#define W1_DDR	DDRD

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

//#define NULL 0x00

volatile float Tist=0;
volatile float Tsoll=8.0;
volatile int timer0=0;
volatile char tempH,tempL,hilf;
volatile int sleep = 1, iMode = 1; // mode: 1 = gas, 2 = DC, 3 = AC
volatile int iLights = 1, iLightsCount = 0, iResetLights = 0, iBuzzer = 0, iBuzzerCount = 0;
int iIsOn = 0;
float fInVoltage = 0, Tsoll_old = 0;;
unsigned char w1_reset(void);
unsigned char w1_bit_io( unsigned char b );
unsigned char w1_byte_wr( unsigned char b );
unsigned char w1_byte_rd( void );
void w1_command( unsigned char command, unsigned char *id );
void start_meas( void );
void myPrintf(float fVal, char*);
void updateLCD(int iLowBat);

int main()
{
	float Tsoll_round;
	float fSpgFlam = 0;
	int iLowBat = 0;

	//TCCR1 |= (1 << CS12) | (1 << CS10);
	
	//MCUCR |= (1 << ISC11) | (1 << ISC10) ;   			//Steigende Flanke von INT1 als ausl�ser
	//GICR |= (1 << INT1);                //Global Interrupt Flag f�r INT1
	
	TCCR0 |= (1 << CS02) | (1 << CS00);

	MCUCR |= (1<<ISC01) | (1<<ISC00);   				//Steigende Flanke von INT0 als ausl�ser
	GICR  |= (1<<INT0);					//Global Interrupt Flag f�r INT0

	TCCR0 |= (1<<CS02)|(1<<CS00);       	// Start Timer 0 with prescaler 1024
	TIMSK |= (1<<TOIE0);                	// Enable Timer 0 overflow interrupt
	
	TCCR1B |= (1<<CS11);       	// Start Timer 1 with prescaler 256
	TCCR1A |= (1<<WGM10);
	TIMSK |= (1<<TOIE1);                	// Enable Timer 1 overflow interrupt
	
	//ADC enable
	ADMUX = (1<<REFS0);              // Set Reference to AVCC and input to ADC0
	ADCSRA = ((1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));    // Enable ADC, set prescaler to 128 // Fadc=Fcpu/prescaler=1000000/16=62.5kHz
	sei();								//Interrupt aktivieren

	ADCSRA |= (1<<ADSC);              // Start the first conversion

	//DDRD &= ~((1<<DDD2) & (1<<DDD3));	// Als Eingang
	PORTD |= (1<<DDD2);					// Int Pull UP
	PORTD |= (1<<DDD3);
	
	DDRD |= (1<<PD0);	//230V realis
	DDRD |= (1<<PD1);	//12V realis
	DDRD |= (1<<PD4);   //buzzer
	DDRD |= (1<<PD7);	//light
	PORTD &= ~(1<<PD0); // off
	PORTD &= ~(1<<PD1); // off
	PORTD &= ~(1<<PD4); // off
	PORTD|=(1<<PD7);

	lcd_init();
	lcd_string("Pflanzbeet Braeu");
	lcd_setcursor(1,2);
	lcd_string("Andreas Dorrer");
	iBuzzer  = 1;
	_delay_ms(1000);
	iBuzzer = 0;
	PORTD &= ~(1<<PD4); // Buzzer off
	_delay_ms(2000);
	lcd_clear();
		
	wdt_enable(WDTO_1S);
	iLightsCount = 0;
	while(1)
	{			
		ADMUX = (1<<REFS0);// ((1<<REFS0) | (1<<REFS0));
		ADCSRA |= (1<<ADSC);
		while(ADCSRA &(1<<ADSC));
		
		Tsoll = (ADC * 5.0/1024.0) * 5;		
		Tsoll_round = (int) Tsoll;
		
		if(Tsoll - Tsoll_round > 0.5)
			Tsoll = Tsoll_round + 0.5;
		else
			Tsoll = Tsoll_round;
			
		if(Tsoll_old > Tsoll + 0.5 || Tsoll_old < Tsoll - 0.5)
			iLights = 1;
		
		if(Tsoll_old != Tsoll && iLights == 1)
		{
			Tsoll_old = Tsoll;
			iResetLights = 1;		
			updateLCD(iLowBat);
		}			
				
		if(iLights == 1)
			PORTD|=(1<<PD7);
		else
			PORTD &= ~(1<<PD7);
					
		if(sleep == 1)
		{
			sleep = 0;	
			if(tempL > 0x80)
			{
				hilf=~tempH;
				Tist=(float)(hilf/(-2));
			}
						
			else
				Tist=(float)tempH/2;

			//// Regler /////////////////////////////////////////
			if(iMode != 1)
			{			
				if(Tist <= (Tsoll_old - 1) || iLowBat == 1)
				{
					if(iMode == 2)
						PORTD &= ~(1<<PD1);		// AUS DC
					else if(iMode == 3)
						PORTD &= ~(1<<PD0);		// AUS AC
					iIsOn = 0;
				}						
				else if(Tist >= (Tsoll_old + 0.5) && iLowBat == 0)
				{
					if(iMode == 2)
						PORTD|=(1<<PD1);		// EIN DC
					else if(iMode == 3)
						PORTD|=(1<<PD0);		// EIN AC
					iIsOn = 1;
				}
			}
			/////////////////////////////////////////////////////
			
			//// Eingangsspannung ///////////////////////////////
			if(iMode != 3)
			{
				ADMUX = (1<<REFS0)|(1<<MUX0);
				ADCSRA |= (1<<ADSC);
				while(ADCSRA &(1<<ADSC));
			
				fInVoltage = (ADC * 5.0/1024.0) / 0.31122;
				
				if(fInVoltage < 10.3 && iMode == 1)
				{	
					iLowBat = 1;
					iBuzzer = 1;
				}
				else if(fInVoltage < 10.9 && iMode == 2)
				{
					iLowBat = 1;
					iBuzzer = 1;
				}
				else
				{
					iLowBat = 0;
					if(iMode == 2 && iIsOn != 0)
						iBuzzer = 0;
					else if(iMode != 2)
					    iBuzzer = 0;
				}								
			}
			else
			{	
				iLowBat = 0;
				iBuzzer = 0;
			}
			/////////////////////////////////////////////////////		
			
			//// Flammkontrolle /////////////////////////////////
			if(iMode == 1)
			{
				ADMUX = (1<<REFS0)|(1<<MUX1);
				ADCSRA |= (1<<ADSC);
				while(ADCSRA &(1<<ADSC));
				
				fSpgFlam = ADC * 5.0/1024.0;	
				if(fSpgFlam > 2.0)
				{	
					iBuzzer = 1;
					iIsOn = 0;
				}
				else
				{	
					if(iLowBat != 1)
						iBuzzer = 0;
					iIsOn = 1;
				}
			}
			/////////////////////////////////////////////////////	
			
			//// Tempkontrolle //////////////////////////////////
			
			
			/////////////////////////////////////////////////////	
			updateLCD(iLowBat);				
		}
		wdt_reset();		
	}
	
	return 0;
}

///////////////////////////////////////////////////////////
/*
ISR(INT1_vect)
{
	if(Tsoll<=30)
	Tsoll+=0.5;
	
	else
	Tsoll=Tsoll;
	
	sleep = 1;
}
*/
ISR(INT0_vect)
{
	if(iLights == 1)
	{	
		if(iMode == 1)
			iMode = 3;
		else if(iMode == 3)
			iMode = 2;
		else if(iMode == 2)
			iMode = 1;	
		PORTD &= ~(1<<PD0); // off AC
		PORTD &= ~(1<<PD1); // off DC
		iResetLights = 1;
	}
	sleep = 1;
	iLights = 1;
}

ISR(TIMER1_OVF_vect)
{
	if(iLights == 1)
	{	
		iLightsCount++;
		if(iLightsCount > 8000)
		{
			iLights = 0;
			iLightsCount = 0;
		}
		if(iResetLights)
		{	
			iLightsCount = 0;
			iResetLights = 0;
		}
	}
	else
	{	
		iLights = 0;
		iResetLights = 0;
		iLightsCount = 0;
	}
	
	if(iBuzzer == 1 && iBuzzerCount < 1000)
		PORTD ^= (1 << PD4);
	else if(iBuzzer == 1 && iBuzzerCount < 3000)
		PORTD &= ~(1<<PD4);
	else if(iBuzzerCount > 4000)
		iBuzzerCount = 0;
	if(iBuzzer == 1)
		iBuzzerCount++;
}

ISR(TIMER0_OVF_vect)
{
	timer0++;
	
	if(timer0>40)	// Temp Messung
	{	timer0=0;
		
		w1_command(READ,NULL);
		tempH=w1_byte_rd();
		tempL=w1_byte_rd();

		start_meas();
		sleep = 1;
	}
}

////////////////////////////////////////////////////////////////////
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

void updateLCD(int iLowBat)
{
	char lcd_Tist[10], lcd_Tsoll[10], lcd_Volt[10];
	static int iBlink = 0;
	//// Ausgabe vorbereiten ///////////////////////////////////
	dtostrf(Tist,2,1,lcd_Tist);
	dtostrf(Tsoll_old,2,1,lcd_Tsoll);
	dtostrf(fInVoltage,2,1,lcd_Volt);
	
	lcd_clear();			
	//// Ausgabe T ist ///////////////////////////////////
	lcd_setcursor(0,1);
	lcd_string("Ti=");
	lcd_string(lcd_Tist);
				
	//// Ausgabe T soll //////////////////////////////////
	lcd_setcursor(1,0);
	lcd_string("  Ts=");
	lcd_string(lcd_Tsoll);
	
	//// Ausgabe Mode ///////////////////////////////////
	lcd_setcursor(0,2);
	if(iMode == 1)
	 lcd_string("M:GA");
	else if(iMode == 2)
	 lcd_string("M:DC");
    else
	 lcd_string("M:AC");
	 
	 //// Ausgabe Satus /////////////////////////////////
	lcd_setcursor(5,2);
	if(iIsOn)
		lcd_string("S:On");
    else
		lcd_string("S:Off");
	  
	//// Ausgabe Satus /////////////////////////////////
	lcd_setcursor(11,2);
	if(iLowBat && iMode != 3 && iBlink == 1)
	{	
		lcd_string("LOW!!");
		iBlink = 0;
	}
	else if(iMode != 3)
	{
		lcd_string(lcd_Volt);
		lcd_string("V");
		iBlink = 1;
	}
	else
		lcd_string("N/A");
}