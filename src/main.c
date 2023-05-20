/* ********************************************************************
FILE                   : main.c

PROGRAM DESCRIPTION    :  Electronic door security system.  
                          At first, default correct password = "123789" is used and enter the password, which is 6 digits.
						  character '#' acts as backspace and '*' acts as char to configure authication password. 
                         1: when OPEN_SW is pressed, to allow access to keypad. password is entered.						  
						 2:  After successuful authentication, Unipolar stepper motor is rotated clockwise from 0 to 90 degrees, then door is opened. 
						 3:  When DOOR_CLOSE switch is pressed Unipolar stepper motor is rotated anticlockwise for 90 degrees to 0 degrees, then door is closed. 
						 4:  wait for OPEN_SW to allow access to keypad. 
						 5:  If first char enter in password is '*', which is invisible in LCD, then system goes into configure mode.
						     then at first password authication process begins and after successfully authication, set password is entered and then confirm 
                             password is entered. 
						 6: If set password and confirm password matches, then new authentication password is set and writtern to internal EEPROM. 
                            if set password and confirm password does not match (no limit), correct password does not change and it goes to Step 1. 
                         7: If door open authentication password is either wrong or timeout occurs, then numbr of retry is incremented by 1. If it exceeds 
                            MAX_RETRY, then microcontroller enters in sleep mode.
                         8: Timer1 is used for time delay by interrupt. 							
                            						  
                          						  

AUTHOR                : K.M.Arun Kumar alias Arunkumar Murugeswaran
	 
KNOWN BUGS            : 

NOTE                  : 
 
USED:                                      
                       
CHANGE LOGS           : 

*****************************************************************************/ 
#include <pic18f4550.h>
#include "Configuration_Header_File.h"
#include "16x2_LCD_4bit_File.h"
#include "EEPROM.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

unsigned char keyfind();            /* function to find pressed key*/

#define write_port LATB             /* latch register to write data on port */
#define read_port PORTB             /* PORT register to read data of port */
#define Direction_Port TRISB
#define NUM_DIGITS         (6)
#define TIMEOUT_PWD_ENTER  (60000UL)
#define TIMEOUT_CONFIG_PWD (65000UL) 
#define TIME_PRESS_START_SW_MSG  (5000UL)
#define TIME_WAIT_DOOR_CLOSE (10000UL)
#define TIME_DOOR_OPENED     (5000UL)
#define TIME_DOOR_CLOSED     (5000UL) 
#define TIME_PWD_CHANGED_DISP (5000UL)
#define MAX_RETRY           (3)
#define F_CPU (8000000/64)
#define MINTHR              8000
#define RESOLUTION          488

#define InternalOsc_8MHz    8000000
#define InternalOsc_4MHz    4000000
#define InternalOsc_2MHz    2000000
#define InternalOsc_1MHz    1000000
#define InternalOsc_500KHz  500000
#define InternalOsc_250KHz  250000
#define InternalOsc_125KHz  125000
#define InternalOsc_31KHz   31000

#define Timer2Prescale_1    1
#define Timer2Prescale_4    4
#define Timer2Prescale_16   16
#define Baud_value(baud_rate) (((float)(F_CPU)/(float)baud_rate)-1)

unsigned char i = 0;
unsigned char correct_pwd[NUM_DIGITS + 1] = "123789";
volatile unsigned long num_milli_sec = 0;
volatile unsigned long max_milli_sec;

typedef enum 
{
	INIT, DOOR_OPENING, DOOR_OPENED, DOOR_CLOSED, WAIT_DOOR_CLOSE, TIME_IN_DOOR_CLOSED, WAIT_OPEN_SW, DISP_ENTER_PWD, TYPING_PWD, BACKSPACE_PROC, DOOR_CLOSING, 
	  PROCESS_PWD, INVALID_PWD, WAIT_DOOR_OPENING, TIMEOUT_NO_PWD, WAIT_PRESS_OPEN_SW, MATCH_CONFIG_PWD, EXCEED_MAX_RETRY, CONFIG_MODE, DISP_PWD_CHANGE
} fsm_door_state_t;

unsigned char keypad[4][3]= {{'1','2','3'},
                             {'4','5','6'},
                             {'7','8','9'},
                             {'*','0','#'}};
							 
volatile fsm_door_state_t fsm_door_state, prev_fsm_door_state;
unsigned char num_retry = 0;

void Timer1_start();
void USART_Init(long);
void USART_TxChar(char);
char USART_RxChar();
void USART_Tx_Str(char *str);
void PWM_Init(); 
void SetDutyCycleTo(float Duty_cycle, int Period); 
int setPeriodTo(unsigned long FPWM);         
char buffer[50];
void Unipolar_Stepper_anticlockwise_Full_step();
void Unipolar_Stepper_clockwise_Full_step();	

typedef struct
{
	unsigned int keypad_enable       : 1;
	unsigned int config_sw_enable    : 1;
	unsigned int open_sw_enable     : 1;
	unsigned int door_close_sw_enable : 1;
	unsigned int confirm_pwd_enable   : 1;
	unsigned int set_pwd_enable       : 1;
	unsigned int pwd_auth_status      : 1;
	unsigned int                      : 1; 
} sws_enable_t;
volatile sws_enable_t switchs_enable;
	
void main() 
{
    int Period;
    char enter_pwd[NUM_DIGITS + 1], config_pwd[NUM_DIGITS + 1], press_key, Data_read, *enter_pwd_str, *match_pwd_str;
    OSCCON = 0x72;
   // RBPU=0;  	/* activate pull-up resistor */
	ADCON1 = 0x0F;
	Direction_Port = 0xF0;              /*PORTD.0-PORTD.3 as a Output Port and PORTD.4-PORTD.7 as a Input Port*/
	TRISAbits.TRISA0 = 0;
	TRISAbits.TRISA1 = 0;
	TRISCbits.TRISC0 = 1;
	TRISCbits.TRISC1 = 0;
	TRISCbits.TRISC2 = 0;
	LCD_Init();                         /* initialize LCD16x2 in 4-bit mode */
	USART_Init(9600);
	MSdelay(50);
	i = 0;
    while(i <= NUM_DIGITS)    
    {    
        Data_read = EEPROM_Read(i);
		if(Data_read != 0xff)
		{
			correct_pwd[i] = Data_read;
		}
        i++;
    }
    sprintf(buffer, "Correct Pwd: %s \r\n", correct_pwd);
    USART_Tx_Str(buffer);
    fsm_door_state = INIT;	
    while(1)
    {
		switch(fsm_door_state)
		{
			case INIT:
                 TMR1ON = 0;
				 switchs_enable.open_sw_enable = 1;
				 switchs_enable.confirm_pwd_enable = 0;
				 switchs_enable.set_pwd_enable = 0;
				 switchs_enable.config_sw_enable = 0;
				 switchs_enable.pwd_auth_status = 0;
	             LCD_Clear();
				 LCD_String_xy(0,0,"Press Open SW");	
                 fsm_door_state = WAIT_OPEN_SW;
                 USART_Tx_Str("INIT -> WAIT_OPEN_SW \r\n");					                  			 
			break;
			case WAIT_OPEN_SW:
			    if(PORTCbits.RC0 == 0 && switchs_enable.open_sw_enable == 1)
	            {
		           while(PORTCbits.RC0 == 0);
		           switchs_enable.open_sw_enable = 0;
				   fsm_door_state =	DISP_ENTER_PWD;
                   USART_Tx_Str("DOOR_CLOSED -> DISP_ENTER_PWD \r\n");			   
	            }
            break;			
			case DISP_ENTER_PWD:
			   	TMR1ON = 0;
				LCD_Clear();
				if(switchs_enable.config_sw_enable == 0)
				{					
		           LCD_String_xy(0,0,"Enter Password");
				   max_milli_sec = TIMEOUT_PWD_ENTER;
				   enter_pwd_str = enter_pwd;
				}
				else
				{
					if(switchs_enable.confirm_pwd_enable == 0)
					{
					   LCD_String_xy(0,0,"Set Password");
					   enter_pwd_str = enter_pwd;
					}
					else
					{
						LCD_String_xy(0,0,"Confirm Pwd");
					    enter_pwd_str = config_pwd;
					}
				    max_milli_sec = TIMEOUT_CONFIG_PWD;                    					 
				}
				LCD_Command(0x0E);
                LCD_Command(0xC0);
                Timer1_start(); 
				num_milli_sec = 0;                
				i = 0;
				switchs_enable.keypad_enable = 1;
				fsm_door_state = TYPING_PWD;
				USART_Tx_Str("DISP_ENTER_PWD -> TYPING_PWD \r\n");	
				if(switchs_enable.confirm_pwd_enable == 0)
				   memset(enter_pwd_str,'\0', NUM_DIGITS + 1);
				PORTAbits.RA1 = 0; 
			    PORTAbits.RA0 = 0; 
		    break;
            case TYPING_PWD:   			
                while(i < NUM_DIGITS) 
		        {	
			       if(switchs_enable.keypad_enable != 1)
				   {
					   break;
				   }
			       press_key = keyfind();	/* find a pressed key */				
                   switch(press_key)
				   {
					   case '\0':
					      continue;
					   break;	  
                       case '#':
                           if (i > 0)
				           {
                                 --i;
								 enter_pwd_str[i] = '\0';
							     LCD_Command(0xC0 + i);
							     LCD_Char(' ');
							     LCD_Command(0x10);
					       }
                        break;
                        case '*':
						   if(i == 0)
						   {
							   USART_Tx_Str("Config mode \r\n");	
							   switchs_enable.config_sw_enable = 1;                                							   
						   }						   
                        break;
                        default:						   
					      LCD_Command(0xC0 + i);
						  LCD_Command(0x06);
                          enter_pwd_str[i] = press_key;					
                          LCD_Char(enter_pwd_str[i]);	/* display pressed key on LCD16x2 */
						  MSdelay(500UL);
						  LCD_Command(0xC0 + i);
						  LCD_Char('X');
                          ++i;
					}					
				}
				enter_pwd_str[i] = '\0';
				sprintf(buffer, "Entered Pwd: %s \r\n", enter_pwd_str);
				USART_Tx_Str(buffer);
				if(switchs_enable.set_pwd_enable == 0)
				{
                    match_pwd_str = correct_pwd;
					if(fsm_door_state == TYPING_PWD)
				    {
                       fsm_door_state = PROCESS_PWD;
				       USART_Tx_Str("TYPING_PWD -> PROCESS_PWD \r\n");					  
				    } 
					break;
				}
                if(switchs_enable.confirm_pwd_enable == 0)
				{
					if(fsm_door_state == TYPING_PWD)
				    {
					     fsm_door_state = MATCH_CONFIG_PWD;
				         USART_Tx_Str("TYPING_PWD -> MATCH_CONFIG_PWD \r\n"); 
					}						 
				}
                else
				{
					if(fsm_door_state == TYPING_PWD)
				    {
					    match_pwd_str = enter_pwd;
					    fsm_door_state = PROCESS_PWD;
				        USART_Tx_Str("TYPING_PWD -> PROCESS_PWD \r\n"); 
					}
				}					
		    break;
            case MATCH_CONFIG_PWD:
               switchs_enable.confirm_pwd_enable = 1;
			   num_retry = 0;
               fsm_door_state = DISP_ENTER_PWD; 
			   USART_Tx_Str("MATCH_CONFIG_PWD -> DISP_ENTER_PWD \r\n");
            break;			
			case PROCESS_PWD: 
				TMR1ON = 0;
				switchs_enable.keypad_enable = 0;								
				LCD_Clear();
				LCD_Command(0x0C);
				if(strcmp(enter_pwd_str, match_pwd_str))
		        {
					USART_Tx_Str("PROCESS_PWD -> INVALID_PWD \r\n");
					fsm_door_state = INVALID_PWD;                    				
		        }
		        else
		        {
					num_retry = 0;
					if(switchs_enable.confirm_pwd_enable == 0 && switchs_enable.config_sw_enable == 1)
					{
					    fsm_door_state = DISP_ENTER_PWD;
					    USART_Tx_Str("PROCESS_PWD -> DISP_ENTER_PWD \r\n");
					    switchs_enable.set_pwd_enable = 1;
						break;
					}					
					if(switchs_enable.config_sw_enable == 0)
					{
			             LCD_String_xy(1,0,"Access Granted");
					     LCD_String_xy(2,0,"WELCOME");
						 Timer1_start(); 
			             num_milli_sec = 0;
                         max_milli_sec = TIME_DOOR_OPENED;
						 USART_Tx_Str("PROCESS_PWD -> WAIT_DOOR_OPENING \r\n");
                         fsm_door_state = WAIT_DOOR_OPENING;
						 break;
					}
					if (switchs_enable.confirm_pwd_enable == 1)
					{
					    switchs_enable.confirm_pwd_enable = 0;
	    				switchs_enable.set_pwd_enable = 0;
		    		    switchs_enable.config_sw_enable = 0;						
                        USART_Tx_Str("PROCESS_PWD -> DISP_PWD_CHANGE \r\n");
						if(strcmp(correct_pwd, enter_pwd_str))
						{
							LCD_String_xy(1,0,"Pwd changed");
						    strcpy(correct_pwd, enter_pwd_str);
							EEPROM_WriteString(0, correct_pwd);
							sprintf(buffer, "changed Pwd: %s write EEPROM \r\n", correct_pwd);
							USART_Tx_Str(buffer);
						}
						else
						{
							LCD_String_xy(1,0,"Config Same Pwd");
						}
						Timer1_start(); 
			            num_milli_sec = 0;
                        max_milli_sec = TIME_PWD_CHANGED_DISP;
                        fsm_door_state = DISP_PWD_CHANGE;						
					}				
		        }
			break;
    		case TIMEOUT_NO_PWD:
			case INVALID_PWD:
			   TMR1ON = 0;
			   LCD_Clear();
			   LCD_Command(0x0C);
			   PORTAbits.RA1 = 1; 
			   PORTAbits.RA0 = 0; 
			   num_milli_sec = 0;	
               if(switchs_enable.confirm_pwd_enable == 0)
			   {				   
    		       ++num_retry;			  
			       if(num_retry >= MAX_RETRY )
			       {
				     	LCD_String_xy(0,0,"Max 3 Retry"); 
                        fsm_door_state = EXCEED_MAX_RETRY;
                     	USART_Tx_Str(" -> EXCEED_MAX_RETRY \r\n");                        				
			       }
			   }
               if(num_retry < MAX_RETRY)
			   {	
		            if(fsm_door_state == INVALID_PWD )
					{
						if(switchs_enable.config_sw_enable == 0)
						     LCD_String_xy(0,0,"Invalid Password");
						else
						   LCD_String_xy(0,0,"Pwd mismatch");
					    USART_Tx_Str("INVALID_PWD -> WAIT_PRESS_OPEN_SW \r\n");	
					}
                    else
					{
                       LCD_String_xy(0,0,"Timeout Entry");	
					   USART_Tx_Str("TIMEOUT_NO_PWD -> WAIT_PRESS_OPEN_SW \r\n");	
					}
					 Timer1_start(); 
			    	 num_milli_sec = 0;
                     max_milli_sec = TIME_PRESS_START_SW_MSG;
					 fsm_door_state = WAIT_PRESS_OPEN_SW;                    										
				}
                switchs_enable.confirm_pwd_enable = 0;
			    switchs_enable.set_pwd_enable = 0;
			    switchs_enable.config_sw_enable = 0; 				
			break;
			case EXCEED_MAX_RETRY:
			    SLEEP();         /*enter in sleep mode*/
			break;
			case WAIT_PRESS_OPEN_SW:
			break;
			case WAIT_DOOR_OPENING:
			break;
			case DISP_PWD_CHANGE:
			break;
           	case DOOR_OPENING:
			   Unipolar_Stepper_clockwise_Full_step();			   
			   fsm_door_state = DOOR_OPENED;
			   PORTAbits.RA0 = 1;
			   USART_Tx_Str("DOOR_OPENING -> DOOR_OPENED \r\n");
			break;
            case DOOR_OPENED:			
			   LCD_Clear();
               LCD_Command(0x0C); 			   
               LCD_String_xy(0,0,"Press Close SW");		   
			   fsm_door_state = WAIT_DOOR_CLOSE;
			   USART_Tx_Str("DOOR_OPENED -> WAIT_DOOR_CLOSE \r\n");
			   switchs_enable.door_close_sw_enable = 1;			   
            break; 
			case WAIT_DOOR_CLOSE:
		    	if(PORTBbits.RB7 == 0 && switchs_enable.door_close_sw_enable == 1)
	            {					
		           while(PORTBbits.RB7 == 0);
		           switchs_enable.door_close_sw_enable = 0;
				   fsm_door_state = DOOR_CLOSING;
				   USART_Tx_Str("WAIT_DOOR_CLOSE -> DOOR_CLOSING \r\n");
	            }
	        break;			
            case DOOR_CLOSING:
                Unipolar_Stepper_anticlockwise_Full_step();
				USART_Tx_Str("DOOR_CLOSING -> DOOR_CLOSED \r\n");
				fsm_door_state = DOOR_CLOSED;
			break;
		    case DOOR_CLOSED:		
			    TMR1ON = 0;
				LCD_Clear();	
				LCD_String_xy(0,0,"Door Closed");
                Timer1_start(); 
			    num_milli_sec = 0;
                max_milli_sec = TIME_DOOR_CLOSED;	 				
			    fsm_door_state = TIME_IN_DOOR_CLOSED;
				PORTAbits.RA0 = 0; 				
			    USART_Tx_Str("DOOR_CLOSED -> TIME_IN_DOOR_CLOSED \r\n"); 								
			break;
			case TIME_IN_DOOR_CLOSED:
			break;            
		}  			
    }
    
}

/*********************Interrupt Service Routine ************************/
void interrupt ISR()
{  
    if(PIR1bits.TMR1IF)
	{	
        ++num_milli_sec;
        if(num_milli_sec < max_milli_sec)
		{
			TMR1=0xF856;
            TMR1ON = 1;
		}
		else
		{
			TMR1ON = 0;
			switch(fsm_door_state)
			{
				case TYPING_PWD:
				    fsm_door_state = TIMEOUT_NO_PWD;
					switchs_enable.keypad_enable = 0;
					USART_Tx_Str("TYPING_PWD -> TIMEOUT_NO_PWD \r\n");	
				break;
				case WAIT_PRESS_OPEN_SW:
				    fsm_door_state = INIT;
					USART_Tx_Str("WAIT_PRESS_OPEN_SW -> INIT \r\n");	
				break;
				case WAIT_DOOR_OPENING:
				    fsm_door_state = DOOR_OPENING;
					USART_Tx_Str("WAIT_DOOR_OPENING -> DOOR_OPENING \r\n");	
				break;
				case TIME_IN_DOOR_CLOSED:
				    fsm_door_state = INIT;
					USART_Tx_Str("TIME_IN_DOOR_CLOSED -> INIT \r\n");	
				break;
				case DISP_PWD_CHANGE:
				    fsm_door_state = INIT;
					USART_Tx_Str("DISP_PWD_CHANGE -> INIT \r\n");	
				break;
			}
		}
      	PIR1bits.TMR1IF=0; /* Make Timer1 Overflow Flag to '0' */	
	}
}

void Timer1_start()
{
    GIE=1;              /* Enable Global Interrupt */
    PEIE=1;             /* Enable Peripheral Interrupt */
    TMR1IE=1;           /* Enable Timer1 Overflow Interrupt */
    TMR1IF=0;
    T1CON=0x80;         /* Enable 16-bit TMR1 Register,No pre-scale,use internal clock,Timer OFF */
    TMR1=0xF856;        /* Load Count for generating delay of 1ms */
    TMR1ON=1;           /* Turn-On Timer1 */
}


unsigned char keyfind()
{  
    if(switchs_enable.keypad_enable == 1)
	{		
         PORTBbits.RB0 = 1;
	     PORTBbits.RB1 = 0;
         PORTBbits.RB2 = 0;       
         PORTBbits.RB3 = 0;
	     if(PORTBbits.RB4 == 1)
	     {
		    while(PORTBbits.RB4 == 1);
		    return keypad[0][0];
	     }
	if(PORTBbits.RB5 == 1)
	{
		while(PORTBbits.RB5 == 1);
		return keypad[0][1];
	}
	if(PORTBbits.RB6 == 1)
	{
		while(PORTBbits.RB6 == 1);
		return keypad[0][2];
	}
    PORTBbits.RB0 = 0;
	PORTBbits.RB1 = 1;
    PORTBbits.RB2 = 0;       
    PORTBbits.RB3 = 0; 
	if(PORTBbits.RB4 == 1)
	{
		while(PORTBbits.RB4 == 1);
		return keypad[1][0];
	}
	if(PORTBbits.RB5 == 1)
	{
		while(PORTBbits.RB5 == 1);
		return keypad[1][1];
	}
	if(PORTBbits.RB6 == 1)
	{
		while(PORTBbits.RB6 == 1);
		return keypad[1][2];
	}
	PORTBbits.RB0 = 0;
	PORTBbits.RB1 = 0;
    PORTBbits.RB2 = 1;       
    PORTBbits.RB3 = 0;
	if(PORTBbits.RB4 == 1)
	{
		while(PORTBbits.RB4 == 1);
		return keypad[2][0];
	}
	if(PORTBbits.RB5 == 1)
	{
		while(PORTBbits.RB5 == 1);
		return keypad[2][1];
	}
	if(PORTBbits.RB6 == 1)
	{
		while(PORTBbits.RB6 == 1);
		return keypad[2][2];
	}
	PORTBbits.RB0 = 0;
	PORTBbits.RB1 = 0;
    PORTBbits.RB2 = 0;       
    PORTBbits.RB3 = 1;
	if(PORTBbits.RB4 == 1)
	{
		while(PORTBbits.RB4 == 1);
		return keypad[3][0];
	}
	if(PORTBbits.RB5 == 1)
	{
		while(PORTBbits.RB5 == 1);
		return keypad[3][1];
	}
	if(PORTBbits.RB6 == 1)
	{
		while(PORTBbits.RB6 == 1);
		return keypad[3][2];
	} 
	}
	return '\0';
	
}

/*****************************USART Initialization*******************************/
void USART_Init(long baud_rate)
{
    float temp;
    TRISC6=0;                       /*Make Tx pin as output*/
    TRISC7=1;                       /*Make Rx pin as input*/
    temp=Baud_value;     
    SPBRG=(int)temp;                /*baud rate=9600, SPBRG = (F_CPU /(64*9600))-1*/
    TXSTA=0x20;                     /*Transmit Enable(TX) enable*/ 
    RCSTA=0x90;                     /*Receive Enable(RX) enable and serial port enable */
}
/******************TRANSMIT FUNCTION*****************************************/ 
void USART_TxChar(char out)
{        
        while(TXIF==0);            /*wait for transmit interrupt flag*/
        TXREG=out;                 /*wait for transmit interrupt flag to set which indicates TXREG is ready
                                    for another transmission*/    
}
/*******************RECEIVE FUNCTION*****************************************/
char USART_RxChar()
{

while(RCIF==0);                 /*wait for receive interrupt flag*/
    if(RCSTAbits.OERR)
    {           
        CREN = 0;
        NOP();
        CREN=1;
    }
    return(RCREG);                  /*receive data is stored in RCREG register and return to main program */
}

void USART_Tx_Str(char *str)
{
	while(*str)
	{
		USART_TxChar(*str);
		++str;
	}
}
unsigned int period = 100;				/* Set period in between two steps of Stepper Motor */

void Unipolar_Stepper_clockwise_Full_step()
{
    //for(int i=0;i<2; i++)   /* Rotate Stepper Motor clockwise with full step sequence; */
	{
		    PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 0;       
            PORTDbits.RD3 = 1;
			MSdelay(period);
			PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 1;
			MSdelay(period);
			/* PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 1;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 0;
			MSdelay(period);
			PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 1;
            PORTDbits.RD2 = 0;       
            PORTDbits.RD3 = 1;
			MSdelay(period);
			PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 1;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 0;
			MSdelay(period); */
	}
}

void Unipolar_Stepper_anticlockwise_Full_step()	
{
		//for(int i=0; i <2;i++)		/* Rotate Stepper Motor Anticlockwise with Full step sequence; Full step angle 7.5 */
		{
            PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 0;
			MSdelay(period);
			PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 0;       
            PORTDbits.RD3 = 1;
			MSdelay(period);
			PORTCbits.RC1 = 1;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 0;       
            PORTDbits.RD3 = 1;
			MSdelay(period);
			/* PORTCbits.RC1 = 1;
	        PORTCbits.RC2 = 0;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 0;
			MSdelay(period);
			PORTCbits.RC1 = 0;
	        PORTCbits.RC2 = 1;
            PORTDbits.RD2 = 1;       
            PORTDbits.RD3 = 0;
			MSdelay(period); */
		}			
 }