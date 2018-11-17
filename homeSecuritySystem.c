// Raman Kooner and Carlos Ornelas
// Final Project
// CECS 262
// Decemeber 12th, 2017

#include <reg51.h>

#define ldata P2
#define COMMAND 0
#define LCD_DATA 1

bit bitCheck;
bit intFlag;
bit laserBreak;

char newValue; 
char oneTest;

unsigned int incNumber;
unsigned int prevNumber;
int          incVal;

sbit inbit0      = P0^0;
sbit LED0        = P1^0;
sbit LED1        = P1^1;
sbit laserSwitch = P1^2;
sbit PINB        = P1^3;
sbit greenLED    = P1^5;
sbit yellowLED   = P1^6;
sbit redLED      = P1^7;
sbit PINA        = P3^2;
sbit rs          = P3^6;
sbit rw          = P3^5;
sbit en          = P3^7;
sbit busy        = P2^7; // connected to DB7 on LCD


//******************************************
// LCD FUNCTIONS
//******************************************
void init_lcd();
void write_to_lcd(unsigned char value, bit mode);
void MSDelay(unsigned int itime);
void lcdready(void);


//******************************************
// STATE FUNCTIONS
//******************************************
void disarmedState(void);
void armedState(void);
void intruderState(void);
void countDownState(void);
void updateState(void);

void Armed(void);       // Displays the word "ARMED" on second row of LCD
void Disarmed(void);    // Displays the word "DISARMED" on second row of LCD
void Intruder(void);    // Displays the word "INTRUDER" on second row of LCD


//******************************************
// COUNTDOWN FUNCTIONS
//******************************************
void Timer(void);
void armTimer(void);
void counter(unsigned int countNumber);
void updatingCount(unsigned int countVal);

void leds(char numberChar, char numberChar2);


//******************************************
// TIMER FUNCTIONS
//******************************************
void T0_M1Delay50ms(void);
void halfSecondDelay(void);
void oneSecondDelay(void);


//******************************************
// INTERRUPT FUNCTIONS
//******************************************
void encoderInterrupt(void);
void laserBeam(void);


// 5 States
enum states
{
	DISARMEDSTATE,
	ARMEDSTATE,
	INTRUDERSTATE,
	COUNTDOWNSTATE,
	UPDATESTATE
};

// Define intial state to be disarmed state
enum states state = DISARMEDSTATE;


//**********************************************************************
// MAIN
//**********************************************************************
void main(void)
{ 
  //IT1 = 1;// Edge trigger for interrupt 
	EX0 = 1;  // Enable external interrupt 0 connected to P3.2
	EX1 = 1;	// Enable external interrupt 2 connected to P3.3
	EA = 1;

	while(1)
	{
	 // Case statement to switch between states
		switch(state)
		{
			case DISARMEDSTATE:
				laserBreak = 0;
				// Start in the disarmed state
				disarmedState();
				
				// intFlag is triggered by external interrupt 
				// if intFlag is detected, then go to update state
				if(intFlag == 1)
				{
					state = UPDATESTATE;
				}
				
				// If dipswitch 1 is on, then go to countdown state
				if (inbit0 == 1)
				{
					state = COUNTDOWNSTATE;
				}
				
				break;
				
			case ARMEDSTATE: 
				
				// If state = ARMEDSTATE, go to armedState()
				armedState();
				// This timer will count down from the number specified in the updateState
				armTimer();
			
				// If dipswitch 1 is set back to 0, then go back to disarmed state
				if(inbit0 == 0)
				{
					state = DISARMEDSTATE;
				}				
				
				// If the laser is broken and we are in the armed state, then go back
				// to countdown state
				if(laserBreak == 1)
				{
					state = COUNTDOWNSTATE;
				}
				
				break;
			
			case INTRUDERSTATE:
				
				// If state = INTRUDERSTATE, then go to intruderState()
				intruderState();
				
				// If dipswitch 1 is set back to 0, then go back to disarmed state
				if(inbit0 == 0)
				{
					state = DISARMEDSTATE;
				}
				
				break;
			
			case COUNTDOWNSTATE:
				
				// If state = COUNTDOWNSTATE, then go to countDownState()
				countDownState();
				
				// If dipswitch 1 is set back to 0, then go back to disarmed state
				if(inbit0 == 0)
				{
					state = DISARMEDSTATE;
				}
				
				// If bitCheck is on, then the previous state was the armed state
				// Go to intruder state
				else if(bitCheck == 1)
				{
					state = INTRUDERSTATE;
				}
				
				// If bitCheck is off, then the previous state was the disarmed state
				// Go to armed state
				else if(bitCheck == 0)
				{
					state = ARMEDSTATE;
				}
				
				break;
				
			case UPDATESTATE:
				
				// Set the interrupt flag to 0
				intFlag = 0;
				
				// Go to the update state
				updateState();
			
				// Always go back to the disarmed state when in update state
   			state = DISARMEDSTATE;
				break;
		}
	} 
}


//**********************************************************************
// STATE FUNCTIONS
//**********************************************************************
void armedState(void)
{
		// Display the word "ARMED"
		Armed();
		
	  // Display the word "Timer: "
		Timer();
		
		// bitCheck used to check which state we are in
		bitCheck = 1;
		
		// Update LEDs and laser
		LED0 = 0;
		LED1 = 0;
		yellowLED = 0;
		redLED = 0;
		greenLED = 0;
		laserSwitch = 1;
}

void disarmedState(void)
{
		// Display the word "DISARMED"
		Disarmed();
		
  	// Dispaly the word "Timer: "
		Timer();
		
		// Update the counter and count down
		updatingCount(incNumber);
	
		// bitCheck used to check which state we are in
		bitCheck = 0;
	
		// Update LEDs and laser
		redLED = 0;
    greenLED = 0;
    yellowLED = 0;
		laserSwitch = 0;
		LED0 = 0;
		LED1 = 0;
}

void intruderState(void)
{
		// Display the word "INTRUDER!!"
		Intruder();
		
	  // Display the word "Timer: "
		Timer();
		
		// Update the LEDs and laser
		LED0 = 1;
		LED1 = 1;
		halfSecondDelay();
		LED0 = 0;
		LED1 = 0;
		laserSwitch = 0;
		halfSecondDelay();
}

void countDownState(void)
{
		// prevNumber is a new variable used to hold the value of the number we 
		// incremented or decremented
		prevNumber = incNumber;
	
		// send the number into the counter to count down
		counter(prevNumber);
}

void updateState(void)
{
		// Increment the global int
		// incVal comes from the increment function
		// incNumber will be used for our encoder count up and our 
		// 			intitial count down value
		incNumber = incNumber + incVal;
		
		// The value of incNumber must be between 7 and 99
		
		// If the value is greater than 99, then set the value back to 99
		if(incNumber > 99)
		{
			incNumber = 99;
		}
		
		// If the value is less than 7, then set the value back to 7
		if(incNumber < 7)
		{
			incNumber = 7;
		}
		
		updatingCount(incNumber);
}


void Armed(void)
{
	// This function will be used to display the word "ARMED"
	unsigned char i = 0;
	unsigned char code armed[]="ARMED";
	init_lcd();

		write_to_lcd(0xC0, COMMAND); 
	while(armed[i]!='\0')
		write_to_lcd(armed[i++],LCD_DATA);
}

void Disarmed(void)
{
	// This function will be used to display the word "ARMED"
	unsigned char i = 0;
	unsigned char code disarmed[]="DISARMED";
	init_lcd();
	
		write_to_lcd(0xC0, COMMAND);
	while(disarmed[i]!='\0')
		write_to_lcd(disarmed[i++],LCD_DATA);
}

void Intruder(void)
{
	// This function will be used to display the word "ARMED"
	unsigned char i = 0;
	unsigned char code intruder[]="INTRUDER!";
	init_lcd();
		write_to_lcd(0xC0, COMMAND);
	while(intruder[i]!='\0')
		write_to_lcd(intruder[i++],LCD_DATA);
}



//**********************************************************************
// COUNTDOWN FUNCTIONS
//**********************************************************************

void counter(unsigned int countNumber)
{
		// Displays the word "Timer: "
		unsigned char code counters[] = "Timer: ";
		unsigned char i = 0;
		int f;
	
		// Create two chars from an integer
		char ten = (countNumber/10) + 48;
		char one = (countNumber%10) + 48;
	
		// Displays the word "Timer: " on the first line
		write_to_lcd(0x80, COMMAND);
			while(counters[i] != '\0')
			write_to_lcd(counters[i++], LCD_DATA);
		
			// For loop which will count down
			for( f = countNumber; f >= 0; f--)
			{
				// If dipswitch 1 is set back to 0, then break and go to 
				// disarmed state
				if(inbit0 == 0)
				{
					break;
				}
				
				// If the LSB is less than the char '0', then we will set the LSB to 9
				// and decrement the MSB
				if(one < 0x30)
				{
					one = 0x39;
					ten--;	
				}
				
				// Write the two chars to the LCD
				write_to_lcd(ten, LCD_DATA);
				write_to_lcd(one, LCD_DATA);
				
				// Decrement the LSB and create a delay
				one--;
				oneSecondDelay();
				
				// This will display the counter on the first line at position 7
				write_to_lcd(0x87, COMMAND);
				
				// Call the leds() function to update the LED lights as we count down
				leds(one, ten);
		}
}

void Timer(void)
{
		// Function is used to display the word "Timer: "
		unsigned char code timer[] = "Timer: ";
		unsigned char i = 0;

		// This will write the word on the first line of the LCD 
		write_to_lcd(0x80, COMMAND);
		while(timer[i] != '\0')
			write_to_lcd(timer[i++], LCD_DATA);
}

void armTimer(void)
{
		// Update the timer 
		updatingCount(incNumber);
}

void updatingCount(unsigned int countVal)
{
		// Convert the integer into two chars for the LCD
		char msb = (countVal/10) + 48;
		char lsb = (countVal%10) + 48;
	
		// Write the two chars to the LCD
		write_to_lcd(msb, LCD_DATA);
		write_to_lcd(lsb, LCD_DATA);
		write_to_lcd(0x87, COMMAND);
}

void leds(char numberChar, char numberChar2)
{		
	
		// If the MSB is equal to 0, then update the LEDS shown below
		if(numberChar2 == 0x30)
		{			
			
			// If the LSB is less than 3, then the red led will be turned on
			if(numberChar <= 0x33)
			{
				redLED = 1;
				yellowLED = 0;
				greenLED = 0;
			}
			
			// If the LSB is between 4 and 6, then the yellow led will be turned on
			if(numberChar >= 0x34 && numberChar <= 0x36)
			{
				redLED = 0;
				yellowLED = 1;
				greenLED = 0;
			}
			
			// If the LSB is over 7, then the green led will be turned on
			if(numberChar >= 0x37)
			{
				redLED = 0;
				yellowLED = 0;
				greenLED = 1;
			}
		}
	
		// If the MSB is over 0, then the green led will be on
		if(numberChar2 > 0x30)
		{
			redLED = 0;
			yellowLED = 0;
			greenLED = 1;
		}
}
		


//**********************************************************************
// LCD FUNCTIONS
//**********************************************************************

// setup LCD for the required display 
void init_lcd()
{
  // Function set format: 001 DL N F  * *
  // Function set value: 00111000
  // DL=1, use 8-bit data bus; N=1,1/16 duty(2 lines),
  // F=0, 5x7 dot character font 
  write_to_lcd(0x38,COMMAND); 
                              
  // Display On/Off Control format: 00001 D C B
  // Display On/Off Control value: 00001110
  // D=1, display on; C=1, cursor on; B=0, cursor blink off
  write_to_lcd(0x0E,COMMAND);

  // Entry mode set format: 000001 I/D S
  // Entry mode set value: 00000110
  // I/D=1, Increment cursor position; S=0, no display shift
  write_to_lcd(0x06,COMMAND);

  // Clear display and returns cursor to the home position(address 0) 
  write_to_lcd(0x01,COMMAND);
}

// write a command or a character to LCD
void write_to_lcd(unsigned char value, bit mode)
{
  lcdready();
  ldata = value; 
  rs = mode; // set for data, reset for command
  rw = 0;
  en = 1;
  MSDelay(1);
  en = 0;
}

void MSDelay(unsigned int itime)
{
   unsigned int i, j;

   for (i=0;i<itime;i++)
     for (j=0;j<1275;j++);
}

// wait for LCD to become ready
void lcdready(void)
{
  busy = 1;
  en = 1;
  rs = 0; // It's a command
  rw = 1; // It's a read command
  while (busy) {
    en = 0;
    MSDelay(1);
    en = 1;
  }
  en=0;
  rw=0;
}



//**********************************************************************
// INTERRUPT FUNCTIONS
//**********************************************************************

void encoderInterrupt (void) interrupt 0
{
		// Set a big flag so we know when we enter the interrupt
		intFlag = 1;
	
		// If PINB is 0, then we hit PINA first so increment
		if(PINB == 0)
		{
			incVal = 1;
		}
	
		// If PINB is 1, then we hit PINB first so decrement 
		if(PINB == 1)
		{
			incVal = -1;
		}
}
	
void laserBeam (void) interrupt 2
{
		// Create a bit flag so we know if entered the interrupt
		laserBreak = 1;
}



//**********************************************************************
// TIMER FUNCTIONS
//**********************************************************************
void T0_M1Delay50ms(void)
{
		// Generate a 50ms clock
		
		TMOD = 0x01;     // We use timer 1
		TL0  = 0xFD;     // Initial value
		TH0  = 0x4B;     
		TR0  = 1;        // Turn on timer
	
		while(TF0 == 0); // Wait for timer to overflow
		TR0  = 0;        // Turns off timer
		TF0  = 0;        // Clears TF0
}

void oneSecondDelay(void)
{
		int f;
		for( f = 0; f < 15; f++ )
		{
			T0_M1Delay50ms();
		}
}

void halfSecondDelay(void)
{
		int f;
		for( f = 0; f < 7; f++ )
		{
			T0_M1Delay50ms();
		}
}
