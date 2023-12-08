#include "main.h"
#include "util.h"
#include "fnd.h"
#include "sysclock.h"
#include "systick.h"
#include "uart.h"
#include "led.h"
#include "xprintf.h"
#include "push_button.h"
#include "i2c_tempsensor.h"

void LedDisplay(uint8_t value);

volatile uint8_t digit3, digit2, digit1, digit0;
volatile uint16_t mode = 0x00;

int8_t cbuf[100];

uint8_t USART2_Buffer_Rx[BufferSize] = {0xFF};
uint32_t Rx2_Counter = 0;
uint8_t newline_flag = 0;
uint8_t cnt1 = 1, cnt2 = 1;

int16_t month = 0;
int16_t day = 0;
int16_t hour = 0;
uint16_t min = 0;
uint16_t sec = 0;
volatile uint16_t segment_counter = 0;
int temp;

// mode == 0x10 MODE : Stop Watch 
// mode == 0x11 press sw1, start to increase count
// mode == 0x12 press sw1 again, the time increase stops
// mode == 0x13 press sw2 to save lap time

// mode == 0x20 MODE : Timer
// mode == 0x21 Timer 00:00
// mode == 0x22 Time delay count
// mode == 0x23 LED delay count
// mode == 0x24 reset

// mode == 0x30 MODE : World Time
// mode == 0x31 time calculation
// mode == 0x32 sw1 -> month day segment
// mode == 0x33 sw2 -> hour min segment
// mode == 0x34 sw1 -> world month day segment
// mode == 0x35 sw2 -> world month day segment

void printStartMenu() 
{
	xprintf("----------[START Menu]----------\n");
  xprintf("|                              |\n");
  xprintf("|    (1) Stop Watch            |\n");
  xprintf("|    (2) Timer                 |\n");
  xprintf("|    (3) World Time            |\n");
  xprintf("|    (4) Temperature Display   |\n");
  xprintf("|    (h) User Manual           |\n");
  xprintf("|    (r) [START Menu]          |\n");
  xprintf("|                              |\n");
  xprintf("--------------------------------\n");
}

void printUserManual()
{
	xprintf("--------------------------[User Manual]-------------------------- \n");
	xprintf("|                                                                |\n");
	xprintf("|       (1) is pressed, the 'Stop Watch' can be used.            |\n");
	xprintf("|       (2) is pressed, the 'Timer' can be used.                 |\n");
	xprintf("|       (3) is pressed, the 'World Time' can be used.            |\n");
	xprintf("|       (4) is pressed, the 'Temperature Display' can be used.   |\n");
	xprintf("|       (h) is pressed, the 'User Manual' can be used.           |\n");
	xprintf("|       (r) is pressed, the '[START Menu]' can be used.          |\n");
	xprintf("|                                                                |\n");
	xprintf("------------------------------------------------------------------\n");
}

void reset()
{
	if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == 'r')
	{
		mode = 0x00;
	}
}

void DateChange()
{
	if (hour <= 0)
  {
		day = day - 1;
    hour = hour + 24;
		if (day <= 0)
    {
			month -= 1;
      if (month <= 0)
      {
				month = 12;
      }
			if (month == 2)
      {
				day = 28;
      }
			else if (month == 4 | month == 6 | month == 9 | month == 11)
      {
				day = 30;
      }
			else
      {
				day = 31;
      }
		}
	}
}

void Time()
{
	cnt1++;
	if(cnt1 == 100)
	{
		cnt1 = 1;
		sec++;
	}
	hour = hour % 25;
	min = min % 61;
	sec = sec % 61;
	
	if(sec >= 60)
	{
		sec = 0;
		min += 1;
		if(min >= 60)
		{
			min = 0;
			hour += 1;
			if(hour >= 24)
			{
				hour = 0;
				day += 1;
				if(month == 2)
				{
					if(day > 28)
					{
						month += 1;
					}
					day = 1;
				}
				else if(month == 4 | month == 6 | month == 9 | month == 11)
				{
					if(day > 30)
					{
						month += 1;
					}
					day = 1;
				}
				else
				{
					if(day > 31)
					{
						month += 1;
					}
					day = 1;
					if(month == 13)
					{
						month = 1;
					}
				}
			}
		}
	}
}

int main(void){
	
	 volatile uint16_t segment_counter = 0;
   volatile uint16_t lab_times[5] = {0};
   volatile uint8_t lab_index = 0;
   volatile uint8_t index = 0;
	 
	 volatile uint16_t timer = 0;
	 volatile uint8_t timer_minutes = 0;
	 volatile uint8_t timer_seconds = 0;
	 volatile uint8_t cnt1 = 1, cnt2 = 1;
	 volatile uint8_t value = 0x00;
	 
   System_Clock_Init(); // Switch System Clock = 180 MHz
   //SystemCoreClockUpdate();
   
   SysTick_Init();
   Led_Initialize();
   pb_initialize();
	 pb_interrupt_init();
   UART_Initialize(USART2);
   Fnd_Initialize();
   I2C_Init(I2C1);
	 
	 TCM75A_Write(0x01,0x00);
	 // Dead loop & program hangs here
   // Light up the LED   
   GPIOB->ODR |= 1 << LED0_PIN; // LED On
   Fnd_Self_Test();
   
   xdev_out(UART_xPutChar);
   xdev_in(UART_xGetChar);
	 
	 printStartMenu();

  // Dead loop & program hangs here
   while(1) {
		 
		 
      /*
      SysTick_Delay(1000);
      LED0_GPIO_Port->BSRR = GPIO_BSRR_PIN_12;
      SysTick_Delay(1000);
      LED0_GPIO_Port->BSRR = GPIO_BSRR_PIN_12 << 16;
      */
      if (newline_flag == 1) {
         newline_flag = 0;
				
				xprintf((const char *)"%s\n", USART2_Buffer_Rx);
				
				if(mode == 0x11)
				{
					if(lab_index == 0)
					{
						xprintf("There is no saved lap time\n");
					}
					if(lab_index >= 6)
					{
						xprintf("***No more lap times could be saved***\n");
					}
					else
					{
						for(index = 0; index < lab_index; index++)
						{
							if(index == 0)
							{										
								xprintf("lab time %d = %d\n", index+1, lab_times[index]);
							}
							else
							{										
								xprintf("lab time %d = %d\n", index+1, lab_times[index]);
							}
						}
					}
					reset();
				}
				
					else if(mode == 0x20)
					{
						reset();
						if(Rx2_Counter == 5 & USART2_Buffer_Rx[2] == ':')
						{
							timer_minutes = ((USART2_Buffer_Rx[0] - 48) * 10) + (USART2_Buffer_Rx[1] - 48);
							timer_seconds = ((USART2_Buffer_Rx[3] - 48) * 10) + (USART2_Buffer_Rx[4] - 48);
							timer = timer_minutes * 60 + timer_seconds;
							
							xprintf("%02d min %02d sec\n", timer_minutes, timer_seconds);
							mode = 0x21;
						}
						else
						{
							xprintf("***Please input the correct value***\n");
						}
					}
					
					else if(mode == 0x30)
					{
						reset();
						if((Rx2_Counter == 11 & USART2_Buffer_Rx[2] == '/')
						& (USART2_Buffer_Rx[5] == '-')
						& (USART2_Buffer_Rx[8] == ':'))
						{
							month = ((USART2_Buffer_Rx[0] - 48) * 10) + (USART2_Buffer_Rx[1] - 48);
							day = ((USART2_Buffer_Rx[3] - 48) * 10) + (USART2_Buffer_Rx[4] - 48);
							hour = ((USART2_Buffer_Rx[6] - 48) * 10) + (USART2_Buffer_Rx[7] - 48);
							min = ((USART2_Buffer_Rx[9] - 48) * 10) + (USART2_Buffer_Rx[10] - 48);
						
							if(month >= 13 | day >= 32 | hour >= 24 | min >= 60)
							{
								xprintf("***Please input the correct value***\n");
							}
							else
							{
							xprintf("Month: %02d\n", month);
							xprintf("Day: %02d\n", day);
							xprintf("Hour: %02d\n", hour);
							xprintf("Min: %02d\n\n", min);
							
							xprintf("Select World Time\n\n");
							
							xprintf("1. London UK\n");
							xprintf("2. San Francisco US\n");
							xprintf("3. Hawaii US\n");
							xprintf("4. Paris France\n");
							xprintf("5. Da Nang Vietnam\n");
							xprintf("6. Thailand\n");
							
							mode = 0x31;
							}
						}
						else
						{
							xprintf("***Please input the correct value***\n");
						}
					}
					
					if(mode == 0x31 | mode == 0x32 | mode == 0x33)
						{
							if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '1')
							{
								xprintf("1. London UK\n"); // -9 hour
								hour -= 9;
								DateChange();
							
								xprintf("London Month: %02d\n", month);
								xprintf("London Day: %02d\n", day);
								xprintf("London Hour: %02d\n", hour);
								xprintf("London Min: %02d\n", min);
								
								mode = 0x36;
							}
							
							else if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '2')
							{
								xprintf("2. San Francisco US\n"); // -17 hour
								hour -= 17;
								DateChange();
							
								xprintf("San Francisco Month: %02d\n", month);
								xprintf("San Francisco Day: %02d\n", day);
								xprintf("San Francisco Hour: %02d\n", hour);
								xprintf("San Francisco Min: %02d\n", min);
								
								mode = 0x37;
							}
						
							else if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '3')
							{
								xprintf("3. Hawaii US\n"); // -19 hour
								hour -= 19;
								DateChange();
							
								xprintf("Hawaii Month: %02d\n", month);
								xprintf("Hawaii Day: %02d\n", day);
								xprintf("Hawaii Hour: %02d\n", hour);
								xprintf("Hawaii Min: %02d\n", min);
								
								mode = 0x38;
							}
						
							else if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '4')
							{
								xprintf("4. Paris France\n"); // -8 hour
								hour -= 8;
								DateChange();
							
								xprintf("Paris Month: %02d\n", month);
								xprintf("Paris Day: %02d\n", day);
								xprintf("Paris Hour: %02d\n", hour);
								xprintf("Paris Min: %02d\n", min);
								
								mode = 0x39;
							}
						
							else if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '5')
							{
								xprintf("5. Da Nang Vietnam\n"); // -2 hour
								hour -= 2;
								DateChange();
							
								xprintf("Da Nang Month: %02d\n", month);
								xprintf("Da Nang Day: %02d\n", day);
								xprintf("Da Nang Hour: %02d\n", hour);
								xprintf("Da Nang Min: %02d\n", min);
								
								mode = 0x310;
							}
						
							else if(Rx2_Counter == 1 & USART2_Buffer_Rx[0] == '6')
							{
								xprintf("6. Thailand\n"); // -2 hour
								hour -= 2;
								DateChange();
							
								xprintf("Thailand Month: %02d\n", month);
								xprintf("Thailand Day: %02d\n", day);
								xprintf("Thailand Hour: %02d\n", hour);
								xprintf("Thailand Min: %02d\n", min);
								
								mode = 0x311;
							}
						}
						reset();
								
						if(mode == 0x00)
						{
						if(Rx2_Counter == 1 & *USART2_Buffer_Rx == '1')
						{
							xprintf("(1) Stop Watch\n");
							mode = 0x10;
						}
						else if(Rx2_Counter == 1 & *USART2_Buffer_Rx == '2')
						{
							xprintf("(2) Timer\n");
							mode = 0x20;
						}
						else if(Rx2_Counter == 1 & *USART2_Buffer_Rx == '3')
						{
							xprintf("(3) World Time\n");
							mode = 0x30;
						}
						else if(Rx2_Counter == 1 & *USART2_Buffer_Rx == '4')
						{
							xprintf("(4) Temperature Display\n");
							mode = 0x40;
						}
						else if(Rx2_Counter == 1 & *USART2_Buffer_Rx == 'h')
						{
							printUserManual();
							mode = 0x00;
						}
						else if(Rx2_Counter == 1 & *USART2_Buffer_Rx == 'r')
						{
							printStartMenu();
							mode = 0x00;
						}
						else
						{
							printUserManual();
							xprintf("***Press the button according to the 'User Manual'***\n");
							mode = 0x00;
						}
					}						
						xmemset(USART2_Buffer_Rx, 0xFF, BufferSize);
						Rx2_Counter = 0;  // Clear Buffer Count         
					}
			
			if(mode == 0x00)
			{
				segment_counter = 0;
				for(index = 0; index < lab_index; index++)
				{
					lab_times[index] = 0;
				}
				lab_index = 0;
				index = 0;
	 
				timer = 0;
				timer_minutes = 0;
				timer_seconds = 0;
				cnt1 = 1, cnt2 = 1;
				value = 0x00;
				
				month = 0;
				day = 0;
				hour = 0;
				min = 0;
				sec = 0;
				temp = 0;
				
				digit3 = 0;
				digit2 = 0;
				digit1 = 0;
				digit0 = 0;
			}

			else if(mode == 0x10)
      {
				digit3 = segment_counter / 1000;
				digit2 = (segment_counter % 1000) / 100;
				digit1 = (segment_counter % 100) / 10;
				digit0 = segment_counter % 10;
      }
      else if(mode == 0x11)
      {
				segment_counter++;
				
				digit3 = segment_counter / 1000;
				digit2 = (segment_counter % 1000) / 100;
				digit1 = (segment_counter % 100) / 10;
				digit0 = segment_counter % 10;
      }
      else if(mode == 0x12)
      {
         segment_counter = 0;
         for(index = 0; index < lab_index; index++)
         {
            lab_times[index] = 0;
         }
         lab_index = 0;
         mode = 0x10;
      }
      else if(mode == 0x13)
      {
         lab_times[lab_index] = segment_counter;
         lab_index++;
				
         if(lab_index >= 6)
					 lab_index = 6;
				 
				 mode = 0x11;
      }
			else if(mode == 0x21)
			{
				digit3 = (timer / 60) / 10;
				digit2 = (timer / 60) % 10;
				digit1 = (timer % 60) / 10;
				digit0 = (timer % 60) % 10;
			}
			else if(mode == 0x22)
			{
				cnt1++;
				if(cnt1 == 100)
				{
					cnt1 = 1;
					timer--;
					if(timer <= 0)
						mode = 0x23;
				}
				digit3 = (timer / 60) / 10;
				digit2 = (timer / 60) % 10;
				digit1 = (timer % 60) / 10;
				digit0 = (timer % 60) % 10;
			}
			else if(mode == 0x23)
			{
				cnt2++;
				if(cnt2 == 20)
				{
					cnt2 = 1;
					value = value ^ 0x01;
				}
				
				LedDisplay(value);
			}
			else if(mode == 0x24)
			{
				timer = 0;
				LedDisplay(0x00);
				digit3 = (timer / 60) / 10;
				digit2 = (timer / 60) % 10;
				digit1 = (timer % 60) / 10;
				digit0 = (timer % 60) % 10;
				mode = 0x20;
			}
			
			else if(mode == 0x32) // month day
			{
				Time();
				digit3 = month / 10; 
				digit2 = month % 10;
				digit1 = day / 10;
				digit0 = day % 10;
			}
			
			else if(mode == 0x33) // hour min
			{
				Time();
				digit3 = hour / 10;
				digit2 = hour % 10;
				digit1 = min / 10;
				digit0 = min % 10;
				
			}
			
			else if(mode == 0x34) // world month day
			{
				Time();
				digit3 = month / 10; 
				digit2 = month % 10;
				digit1 = day / 10;
				digit0 = day % 10;
			}
			
			else if(mode == 0x35) // world hour min
			{
				Time();
				digit3 = hour / 10;
				digit2 = hour % 10;
				digit1 = min / 10;
				digit0 = min % 10;
			}
			
			else if(mode == 0x40)
			{
				temp=TCM75A_Read(0x00);
				display_temp(temp);//display temperature to FND
			}
			
			SysTick_Delay(10);
	 }
 }

void EXTI2_IRQHandler(void) {
    if ((EXTI->PR & EXTI_PR_PR2) == EXTI_PR_PR2) {
      if(mode == 0x10) // MODE 1 initial state
				{
					mode = 0x11; //sw1 -> increase from 0sec
				}
				else if(mode == 0x11)
        {
					mode = 0x10;
        }
				else if(mode == 0x21)
				{
					mode = 0x22;
				}
				else if(mode == 0x31 | mode == 0x33)
				{
					mode = 0x32;
				}
				else if(mode == 0x35 | mode == 0x36 | mode == 0x37 | mode == 0x38 | mode == 0x39 | mode == 0x310 | mode == 0x311)
				{
					mode = 0x34;
				}
        // Cleared flag by writing 1
        EXTI->PR |= EXTI_PR_PR2;
		}
}

void EXTI4_IRQHandler(void) {
    if ((EXTI->PR & EXTI_PR_PR4) == EXTI_PR_PR4) {
         if(mode == 0x10) 
         {
            mode = 0x12; 
         }
         else if(mode == 0x11)
         {
            mode = 0x13;
         }
				 else if(mode == 0x21 | mode == 0x22 | mode == 0x23)
				 {
					 mode = 0x24;
				 }
				 else if(mode == 0x31 | mode == 0x32)
				 {
					 mode = 0x33;
				 }
				 else if(mode == 0x34 | mode == 0x36 | mode == 0x37 | mode == 0x38 | mode == 0x39 | mode == 0x310 | mode == 0x311)
				{
					mode = 0x35;
				}
        // Cleared flag by writing 1
        EXTI->PR |= EXTI_PR_PR4;
    }
}
