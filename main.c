#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#include "lcd-routines.h"
#include "ds18b20/ds18b20.h"
#include "rotary/rotary_encoder.h"
#include "menu/menu.h"
#include "eeprom/eeprom.h"
#include "globals.h"

volatile uint16_t iInactiveCount = 0;
volatile uint8_t uiRodaryPressActive = 0, uiRodaryPush = 0;
volatile uint8_t uiToggle = 1;
volatile uint8_t uiLowBat = 1;

volatile STATES_t state = MEASURE_ALL;
OPERATION_MODES_t mode = GAS;
float fSetTemp = 0;

volatile int timer0 = 0;
volatile int iLights = 0;
int iIsOn = 0;
float fInVoltage = 0;
volatile float fTCurrent;
uint8_t uiTempError;
uint8_t uiBuzzer = 0;
uint8_t uiFlameDetectedForFirstTime = 0;

// eeprom values:
uint8_t uiLongPressTime = 60;

void myPrintf(float fVal, char *);
void updateLCD();
void measureAll();
void setLight(uint8_t on);
void switchToEdit();
void switchToMenu();
void wakeUp();
void readDCVoltage(float *fInVoltageDC);
void readGasVoltage(float *fInVoltageGas);
void setBuzzer(uint8_t on);
void checkBuzzer();

// Pinout:
// PD0 = 230V relay
// PD1 = 12V relay
// PD2 = Button Rotary
// PD3 = Rotary A
// PD4 = Rotary B
// PD5 = buzzer
// PD6 = temp sensor
// PD7 = light

// PC0 = Drill Bat Voltage
// PC1 = DC Input Voltage
// PC2 = Flame Control

#define PHASE_A (PIND & 1 << PD3)
#define PHASE_B (PIND & 1 << PD4)

int main()
{
	int8_t enc_new_delta = 0;

	// Read value from eeprom
	fSetTemp = readSetTemp();
	mode = readMode();

	MCUCR |= (1 << ISC11) | (1 << ISC10); // Steigende Flanke von INT1 als ausloeser
	GICR |= (1 << INT1);				  // Global Interrupt Flag fuer INT1

	// Timer 0
	// start timer 0 with prescaler 1024
	TCCR0 |= (1 << CS02) | (1 << CS00);
	// enable timer 0 overflow interrupt
	TIMSK |= (1 << TOIE0);

	// ADC
	// Set reference to AVCC and input to ADC0
	ADMUX = (1 << REFS0);
	// Enable ADC, set prescaler to 128 // Fadc=Fcpu/prescaler=1000000/16=62.5kHz
	ADCSRA = ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));
	// Start the first conversion
	ADCSRA |= (1 << ADSC);

	DDRD |= (1 << PD0);	  // 230V relay
	DDRD |= (1 << PD1);	  // 12V relay
	DDRD |= (1 << PD5);	  // buzzer
	DDRD |= (1 << PD7);	  // light
	PORTD &= ~(1 << PD0); // 230V relay off
	PORTD &= ~(1 << PD1); // 12V relay off
	PORTD &= ~(1 << PD5); // buzzer off
	PORTD |= (1 << PD7);  // on

	encode_init();
	w1_reset();
	lcd_init();
	setLight(1);

	// enable interrupts
	sei();

	lcd_setcursor(0, 1);
	lcd_string("Pflanzbeet Braeu");
	lcd_setcursor(1, 2);
	lcd_string("Andreas Dorrer");

	// wait for spg stabilize
	_delay_ms(1000);

	// auto detect mode
	float fInVoltageDC;
	readDCVoltage(&fInVoltageDC);

	float fInVoltageGas;
	readGasVoltage(&fInVoltageGas);

	if (fInVoltageDC > 5)
	{
		mode = DC;
	}
	else if (fInVoltageGas > 5)
	{
		mode = GAS;
	}
	else
	{
		mode = AC;
	}

	iInactiveCount = 0;

	wdt_enable(WDTO_1S);
	set_sleep_mode(SLEEP_MODE_IDLE);

	while (1)
	{
		switch (state)
		{
		case SLEEP:
		{
			GIFR |= (1 << INTF0); // reset interrupt flag bit otherwise ISR would be called right after enableing because this bit gets set everytime
			GICR |= (1 << INT1);
			stop_encode(); // stop encode timer as not needed right now
			setLight(0);
			RodaryPush = &wakeUp;
			// RodaryLongPush = NULL;
			sleep_mode();
		}
		break;
		case IDLE:
		{
			if (iInactiveCount > 200)
			{
				iInactiveCount = 0;
				setLight(0);
				state = SLEEP;
			}
		}
		break;
		case WAKE_UP:
		{

			start_encode();
			GICR &= ~(1 << INT1); // stop ext interrup not needed right now
			setLight(1);
			RodaryPush = &switchToEdit;
			// RodaryLongPush = &switchToMenu;
			state = IDLE;
		}
		break;
		case MEASURE_ALL:
		{
			measureAll();
			updateLCD();

			if (iLights > 0)
			{
				state = IDLE;
			}
			else
			{
				state = SLEEP;
			}
		}
		break;
		case EDIT_TEMP_MODE:
		{
			setLight(1);
			if (iInactiveCount > 500)
			{
				iInactiveCount = 0;
				resetMenu();
				state = MEASURE_ALL;
			}
		}
		break;
		case MENU:
		{
			setLight(1);
			if (iInactiveCount > 500)
			{
				iInactiveCount = 0;
				resetMenu();
				state = MEASURE_ALL;
			}
		}
		}

		if (uiRodaryPush == 1)
		{
			uiRodaryPressActive = 0;
			uiRodaryPush = 0;
			iInactiveCount = 0;
			if (RodaryPush)
				(*RodaryPush)();
		}
		/*else if (uiRodaryPush == 2 || (uiRodaryPressActive && iInactiveCount > (uiLongPressTime * 2) - 1))
		{
			uiRodaryPressActive = 0;
			uiRodaryPush = 0;
			iInactiveCount = 0;
			if (RodaryLongPush)
				(*RodaryLongPush)();
		}*/

		enc_new_delta = encode_read4();
		if (enc_new_delta != 0)
		{
			iInactiveCount = 0;
			if (RodaryTick)
				(*RodaryTick)(enc_new_delta);
		}

		wdt_reset();
	}
	return 0;
}

ISR(INT0_vect)
{
	if (!(PIND & (1 << PD2)))
	{
		iInactiveCount = 0;
		uiRodaryPressActive = 1;
		iLights = 1;
	}
	else if (uiRodaryPressActive)
	{
		if (iInactiveCount > (uiLongPressTime * 2) - 1)
			uiRodaryPush = 2;
		else
			uiRodaryPush = 1;
		iLights = 1;
		iInactiveCount = 0;
	}
}

ISR(INT1_vect)
{
	iInactiveCount = 0;
	state = WAKE_UP;
}

ISR(TIMER0_OVF_vect)
{
	timer0++;

	if (timer0 % 20 == 0 && Blink)
	{
		uiToggle = !uiToggle;
		(*Blink)(uiToggle);

		// Buzzer
		if (uiBuzzer > 0)
		{
			// toogle buzzer
			PORTD ^= (1 << PD5);
		}
	}

	// Meassure all if in idle or sleep
	if (timer0 > 60 && (state == SLEEP || state == IDLE))
	{
		timer0 = 0;
		state = MEASURE_ALL;
	}

	if (iLights > 0)
	{
		iInactiveCount++;
	}
}

void setLight(uint8_t on)
{
	if (on > 0)
	{
		PORTD |= (1 << PD7);
		iLights = 1;
	}
	else
	{
		PORTD &= ~(1 << PD7);
		iLights = 0;
	}
}

void updateLCD()
{
	char lcd_Tist[10], lcd_Tsoll[10], lcd_Volt[10];
	static int iBlink = 0;
	// prepare string
	dtostrf(fTCurrent, 2, 1, lcd_Tist);
	dtostrf(fSetTemp, 2, 1, lcd_Tsoll);
	dtostrf(fInVoltage, 2, 1, lcd_Volt);

	lcd_clear();
	// print current temp
	lcd_setcursor(0, 1);
	lcd_string("Ti=");
	if (uiTempError > 0)
		lcd_string("Err");
	else
		lcd_string(lcd_Tist);

	// print set temp
	lcd_setcursor(9, 1);
	lcd_string("Ts=");
	lcd_string(lcd_Tsoll);

	// print mode
	lcd_setcursor(0, 2);
	if (mode == GAS)
		lcd_string("M:GA");
	else if (mode == DC)
		lcd_string("M:DC");
	else
		lcd_string("M:AC");

	// print status
	lcd_setcursor(5, 2);
	if (iIsOn)
		lcd_string("S:On");
	else
		lcd_string("S:Off");

	// print input voltage
	lcd_setcursor(11, 2);
	if (uiLowBat && mode != AC && iBlink == 1)
	{
		lcd_string("LOW!!");
		iBlink = 0;
	}
	else if (mode != AC)
	{
		lcd_string(lcd_Volt);
		lcd_string("V");
		iBlink = 1;
	}
	else
		lcd_string("N/A");
}

void measureAll()
{
	if (read_meas(&fTCurrent) > 0)
		uiTempError = 1;
	else
		uiTempError = 0;

	start_meas();

	float fSpgFlam = 0;

	//// Temp Controller ////////////////////////////////
	if (mode != GAS)
	{
		if (fTCurrent <= (fSetTemp - 1) || uiLowBat == 1)
		{
			if (mode == DC)
				PORTD &= ~(1 << PD1); // AUS DC
			else if (mode == AC)
				PORTD &= ~(1 << PD0); // AUS AC
			iIsOn = 0;
		}
		else if (fTCurrent >= (fSetTemp + 0.5) && uiLowBat == 0)
		{
			if (mode == DC)
				PORTD |= (1 << PD1); // EIN DC
			else if (mode == AC)
				PORTD |= (1 << PD0); // EIN AC
			iIsOn = 1;
		}

		if (mode == DC)
		{
			PORTD &= ~(1 << PD0); // AUS AC
		}
		else if (mode == AC)
		{
			PORTD &= ~(1 << PD1); // AUS DC
		}
	}
	else
	{
		PORTD &= ~(1 << PD0); // AUS AC
		PORTD &= ~(1 << PD1); // AUS DC
	}

	//// Input Voltage Check ////////////////////////////
	if (mode != AC)
	{
		if (mode == DC)
		{
			readDCVoltage(&fInVoltage);
		}
		else if (mode == GAS)
		{
			readGasVoltage(&fInVoltage);
		}

		if (fInVoltage < 15 && mode == GAS)
		{
			uiLowBat = 1;
		}
		else if (fInVoltage < 10.5 && mode == DC)
		{
			uiLowBat = 1;
		}
		else
		{
			uiLowBat = 0;
		}
	}
	else
	{
		uiLowBat = 0;
	}

	//// Flame observation //////////////////////////////
	if (mode == GAS)
	{
		ADMUX = (1 << REFS0) | (1 << MUX1);
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC))
			;

		fSpgFlam = ADC * 5.0 / 1024.0;
		if (fSpgFlam > 2.0)
		{
			iIsOn = 0;
		}
		else
		{
			iIsOn = 1;
			uiFlameDetectedForFirstTime = 1;
		}
	}
}

void switchToEdit()
{
	state = EDIT_TEMP_MODE;
	enableSetTempAndModeEdit();
}

void switchToMenu()
{
	state = MENU;
	drawMenu();
}

void wakeUp()
{
	state = WAKE_UP;
}

void readDCVoltage(float *fInVoltageDC)
{
	ADMUX = (1 << REFS0) | (1 << MUX0);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC))
		;
	*fInVoltageDC = (ADC * 5.0 / 1023.0) * 3.2;
}

void readGasVoltage(float *fInVoltageGas)
{
	ADMUX = (1 << REFS0);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC))
		;
	*fInVoltageGas = (ADC * 5.0 / 1023.0) * 4.25;
}

void setBuzzer(uint8_t on)
{
	if (on > 0)
	{
		uiBuzzer = 1;
		PORTD |= (1 << PD5);
	}
	else
	{
		uiBuzzer = 0;
		PORTD &= ~(1 << PD5);
	}
}

void checkBuzzer()
{
	// check if buzzer should be on
	// Criteria:
	// - Low battery
	// - Temp error
	// - Flame went out after detected for the first time
	if ((uiLowBat > 0 && mode != AC) || uiTempError > 0 || (uiFlameDetectedForFirstTime > 0 && iIsOn < 1))
	{
		setBuzzer(1);
	}
	else
	{
		setBuzzer(0);
	}
}