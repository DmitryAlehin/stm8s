#include "stm8s.h"

#define E GPIOC, GPIO_PIN_3
#define RS GPIOC, GPIO_PIN_4
#define D7 GPIOD, GPIO_PIN_1
#define D6 GPIOC, GPIO_PIN_7
#define D5 GPIOC, GPIO_PIN_6
#define D4 GPIOC, GPIO_PIN_5

#define E_set() (GPIO_WriteHigh(E))
#define E_reset() (GPIO_WriteLow(E))
#define RS_set() (GPIO_WriteHigh(RS))
#define RS_reset() (GPIO_WriteLow(RS))
#define D7_set() (GPIO_WriteHigh(D7))
#define D7_reset() (GPIO_WriteLow(D7))
#define D6_set() (GPIO_WriteHigh(D6))
#define D6_reset() (GPIO_WriteLow(D6))
#define D5_set() (GPIO_WriteHigh(D5))
#define D5_reset() (GPIO_WriteLow(D5))
#define D4_set() (GPIO_WriteHigh(D4))
#define D4_reset() (GPIO_WriteLow(D4))


typedef struct
{
  unsigned Select       : 1;
  unsigned Left         : 1;
  unsigned Right        : 1;
  unsigned Up           : 1;
  unsigned Down         : 1;
  unsigned Reserved     : 3;
}Events_t;

typedef struct
{
  uint8_t Seconds;
  uint8_t Minutes;
  uint8_t Hours;
}Time_t;

typedef struct
{
  uint8_t Year;
  uint8_t Month;
  uint8_t Day;
  uint8_t Date;
}Date_t;

typedef enum
{
  TIME_MENU_MAIN = 0,
  ALARM_1_MAIN,
  ALARM_2_MAIN,
  TIME_SETTING,
  ALARM_1_SETTING,
  ALARM_2_SETTING
}Menu_States_t;

Events_t Events;
Time_t Time;
Date_t Date;
Menu_States_t Menu_State;
void LCD_Init(void);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_SetPos(uint8_t x, uint8_t y);
void LCD_Data(uint8_t dt);
void LCD_String(char* st);
void LCD_SendChar(char ch);
void LCD_Clear(void);
void Delay(uint32_t delay);
void delay(void);
void Clear_Events(void);
void Init_GPIO(void);
void Init_Timers(void);
uint8_t BCD_TO_DEC(uint8_t data);
uint8_t DEC_TO_BCD(uint8_t data);
uint8_t i2c_mem_read_arr(uint16_t device_address, uint16_t mem_address_start, uint8_t* data_array, uint16_t NumByteToRead);
void GetTimeDate(void);
static uint32_t TimingDelay = 0;
static uint16_t MenuTimer;
static uint8_t aTxBuffer[8];
static uint8_t receive_data[7];
int main( void )
{  
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); // инициализация тактирования
  Init_GPIO();
  Init_Timers();
  I2C_DeInit();
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C,ENABLE);
  I2C_Init(I2C_MAX_STANDARD_FREQ, 0xA0, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, 16);
  I2C_Cmd( ENABLE);
 
  
  ADC1_DeInit();
  ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_3, ADC1_PRESSEL_FCPU_D8,
          ADC1_EXTTRIG_TIM,DISABLE, ADC1_ALIGN_RIGHT,  ADC1_SCHMITTTRIG_CHANNEL2, DISABLE);
  ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
  
  
  
  enableInterrupts();
  TIM4_Cmd(ENABLE);
  
  Delay(100);
  LCD_Init();
  LCD_Clear();
  LCD_SetPos(0, 0);
  LCD_String(" ALARM  PROJECT");
  LCD_SetPos(0, 1);
  LCD_String("v0.1 by Dimka :)");  
  ADC1_StartConversion();
  MenuTimer = 0xFFFF;
  Menu_State = TIME_MENU_MAIN;
  //LCD_Clear();
  while(1)
  { 
    switch(Menu_State)
    {
      case TIME_MENU_MAIN:
        if(Events.Right)
        {
          Clear_Events();
          Menu_State = ALARM_1_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(Events.Left)
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(MenuTimer == 0xFFFF)
        {
          ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
          MenuTimer = 64535;
          LCD_Clear();
          LCD_SetPos(0, 0);
          GetTimeDate();
          LCD_SendChar((char) ((Date.Date/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Date%10) + 0x30);
          LCD_SendChar(':');
          
          LCD_SendChar((char) ((Date.Month/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Month%10) + 0x30);
          LCD_SendChar(':');
          
          LCD_SendChar((char) ((Date.Year/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Year%10) + 0x30);
          
          LCD_String("   ");
          LCD_SendChar((char) ((Time.Hours/10)%10) + 0x30);
          LCD_SendChar((char) (Time.Hours%10) + 0x30);
          LCD_SendChar(':');
          
          LCD_SendChar((char) ((Time.Minutes/10)%10) + 0x30);
          LCD_SendChar((char) (Time.Minutes%10) + 0x30);
          //LCD_SendChar(':');
          
          //LCD_SendChar((char) ((Time.Seconds/10)%10) + 0x30);
          //LCD_SendChar((char) (Time.Seconds%10) + 0x30);
          
          LCD_SetPos(1, 1);
          switch(Date.Day)
          {
            case 1:
              LCD_String("    Monday");
              break;
            case 2:
              LCD_String("Tuesday");
              break;
            case 3:
              LCD_String("Wednesday");
              break;
            case 4:
              LCD_String("Thursday");
              break;
            case 5:
              LCD_String("Friday");
              break;
            case 6:
              LCD_String("Saturday");
              break;
            case 7:
              LCD_String("Sunday");
              break;
          }
        }
          //Delay(600);
        break;
      case ALARM_1_MAIN:
        if(Events.Right)
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(Events.Left)
        {
          Clear_Events();
          Menu_State = TIME_MENU_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(MenuTimer == 0xFFFF)
        {
           ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
           MenuTimer = 64535;
           LCD_Clear();
           LCD_SetPos(0, 0);
           LCD_String("    ALARM 1");
           //Delay(10);
        }
        break;
      case ALARM_2_MAIN:
        if(Events.Right)
        {
          Clear_Events();
          Menu_State = TIME_MENU_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(Events.Left)
        {
          Clear_Events();
          Menu_State = ALARM_1_MAIN;
          MenuTimer = 0xFFFF - 10;
          //ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
        }
        if(MenuTimer == 0xFFFF)
        {
           ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
           MenuTimer = 64535;
           LCD_Clear();
           LCD_SetPos(0, 0);
           LCD_String("    ALARM 2");
           //Delay(10);
        }
        break;
    }
    
    
    
  }
  
}

void LCD_Init(void)
{  
    Delay(40);
    RS_reset();
    LCD_WriteData(3);
    E_set();
    delay();
    E_reset();
    Delay(1);
    LCD_WriteData(3);
    E_set();
    delay();
    E_reset();
    Delay(1);
    LCD_WriteData(3);
    E_set();
    delay();
    E_reset();
    Delay(1);
    LCD_WriteCmd(0x28);//??? ?, 2 ??(? ?? ?????? ??4 ?? ????8	
    Delay(1);
    LCD_WriteCmd(0x28);//? ??? ????
    LCD_WriteCmd(0x0C);//?????? (D=1), ??? ??????
    Delay(1);
    LCD_WriteCmd(0x01);//?????
    Delay(2);
    LCD_WriteCmd(0x06);//????
    Delay(1);
    LCD_WriteCmd(0x02);//???? ??????????
    Delay(2);
}

void LCD_WriteCmd(uint8_t cmd)
{ 
  uint8_t temp = 0;
  RS_reset();
  LCD_WriteData(cmd >> 4);
  E_set();
  delay();
  E_reset();
  delay();
  LCD_WriteData((cmd));
  E_set();
  delay();
  E_reset();
  delay();
}

void LCD_WriteData(uint8_t data)
{
  if(((data>>3)&0x01) == 1) {D7_set();} else {D7_reset();}
  if(((data>>2)&0x01) == 1) {D6_set();} else {D6_reset();}
  if(((data>>1)&0x01) == 1) {D5_set();} else {D5_reset();}
  if((data&0x01) == 1) {D4_set();} else {D4_reset();}
}

void Delay(uint32_t delay)
{
  TimingDelay = delay;

  while (TimingDelay);
}

void LCD_SetPos(uint8_t x, uint8_t y)
{
    switch(y)
    {
        case 0:
          LCD_WriteCmd(x|0x80);
          Delay(1);
          break;
        case 1:
          LCD_WriteCmd((0x40+x)|0x80);
          Delay(1);
          break;
    }
}

void LCD_Clear(void)
{
	LCD_WriteCmd(0x01);
	Delay(2);
}
//------------------------------------------------
void LCD_SendChar(char ch)
{
	LCD_Data((uint8_t)ch);
	delay();
}
//------------------------------------------------
void LCD_String(char* st)
{
	uint8_t i=0;
	while(st[i]!=0)
	{
          LCD_Data(st[i]);
          delay();
          i++;
	}
}
void LCD_Data(uint8_t dt)
{
  uint8_t temp = 0;
	RS_set();
	LCD_WriteData(dt >> 4);
	E_set();
	delay();
	E_reset();
        delay();
	LCD_WriteData((dt));
	E_set();
	delay();
	E_reset();
        delay();
}

void delay(void)
{
  uint16_t i, j;
  for(i = 0; i < 20; i++)
  {  
    
  }
}

void Clear_Events(void)
{
  Events.Down = 0;
  Events.Left = 0;
  Events.Right = 0;
  Events.Select = 0;
  Events.Up = 0;
}

void Init_GPIO(void)
{
  GPIO_Init(D4, GPIO_MODE_OUT_PP_LOW_SLOW); 
  GPIO_Init(D5, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(D6, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(D7, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(RS, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(E, GPIO_MODE_OUT_PP_LOW_SLOW);
}

void Init_Timers(void)
{
  TIM2_TimeBaseInit(TIM2_PRESCALER_64, 600);
  TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 200, TIM2_OCPOLARITY_HIGH);
  TIM2_OC1PreloadConfig(ENABLE); 
  
  TIM4_TimeBaseInit(TIM4_PRESCALER_128, 124);
  TIM4_ClearFlag(TIM4_FLAG_UPDATE);
  TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
}

uint8_t BCD_TO_DEC(uint8_t data)
{
	return ((data>>4)*10+(0x0F&data));
}

uint8_t DEC_TO_BCD(uint8_t data)
{
	return ((data/10)<<4)|(data%10);
}

uint8_t i2c_mem_read_arr(uint16_t device_address, uint16_t mem_address_start, uint8_t* data_array, uint16_t NumByteToRead)
{
	uint16_t time_out= 5000;

	/* If I2C bus is busy wait ultil it is free */
	while (I2C_GetFlagStatus(I2C_FLAG_BUSBUSY)&& (--time_out));
	if(!time_out) return 1;

	/* Send Start Condition then wait event EV5 */
	I2C_GenerateSTART(ENABLE);
	while ((!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))&& (--time_out));
	if(!time_out) return 1;

	/* Send device address to write data then wait event EV6 */
	I2C_Send7bitAddress(device_address, I2C_DIRECTION_TX);
	while((!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))&& (--time_out));
	if(!time_out) return 1;

	/* Send memory address pointer then wait event EV8_2 */

        I2C_SendData((uint8_t)mem_address_start);	

	while((!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))&& (--time_out));
	if(!time_out) return 1;

	/* Send Repeat Start Condition then wait event EV5 */
	I2C_GenerateSTART(ENABLE);
	while ((!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))&& (--time_out));
	if(!time_out) return 1;

	/* Send device address to read data then wait event EV6 */
	I2C_Send7bitAddress(device_address, I2C_DIRECTION_RX);
	while((!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))&& (--time_out));
	if(!time_out) return 1;

	if(NumByteToRead == 1)
	{
		/* Disable Acknowledgement */
		I2C_AcknowledgeConfig(I2C_ACK_NONE);

		/* Send STOP Condition */
		I2C_GenerateSTOP(ENABLE);

		/* Poll on RxNE Flag */
		while ((I2C_GetFlagStatus(I2C_FLAG_RXNOTEMPTY) == RESET) && (--time_out));
		if(!time_out) return 1;

		/* Read a byte from the Slave */
		*data_array = I2C_ReceiveData();
	}
	else if(NumByteToRead == 2)
	{
		disableInterrupts();

		/* Poll on RxNE Flag */
		while ((I2C_GetFlagStatus(I2C_FLAG_RXNOTEMPTY) == RESET)&& (--time_out));
		if(!time_out) return 1;

		*data_array = I2C_ReceiveData();

		data_array++;

		/* Disable Acknowledgement */
		I2C_AcknowledgeConfig(I2C_ACK_NONE);

		/* Send STOP Condition */
		I2C_GenerateSTOP(ENABLE);

		/* Poll on RxNE Flag */
		while ((I2C_GetFlagStatus(I2C_FLAG_RXNOTEMPTY) == RESET)&& (--time_out));
		if(!time_out) return 1;

		enableInterrupts();
		/* Read a byte from the Slave */
		*data_array = I2C_ReceiveData();
	}
	else
	{
		while (NumByteToRead)
		{
			if (NumByteToRead != 3) /* Receive bytes from first byte until byte N-3 */
			{
				while ((I2C_GetFlagStatus(I2C_FLAG_TRANSFERFINISHED) == RESET)&& (--time_out));
				if(!time_out) return 1;

				/* Read a byte from the Slave */
				*data_array = I2C_ReceiveData();

				/* Point to the next location where the byte read will be saved */
				data_array++;

				/* Decrement the read bytes counter */
				NumByteToRead--;
			}
			if (NumByteToRead == 3)  /* it remains to read three data: data N-2, data N-1, Data N */
			{
				/* Data N-2 in DR and data N -1 in shift register */

				/* Poll on BTF */
				while ((I2C_GetFlagStatus(I2C_FLAG_TRANSFERFINISHED) == RESET)&& (--time_out));
				if(!time_out) return 1;

				/* Clear ACK */
				I2C_AcknowledgeConfig(I2C_ACK_NONE);

				/* Disable general interrupts */
				disableInterrupts();

				/* Read Data N-2 */
				*data_array = I2C_ReceiveData();

				/* Point to the next location where the byte read will be saved */
				data_array++;

				/* Program the STOP */
				I2C_GenerateSTOP(ENABLE);

				/* Read DataN-1 */
				*data_array = I2C_ReceiveData();

				/* Enable General interrupts */
				enableInterrupts();

				/* Point to the next location where the byte read will be saved */
				data_array++;

				while ((I2C_GetFlagStatus(I2C_FLAG_RXNOTEMPTY) == RESET)&& (--time_out)); /* Poll on RxNE */
				if(!time_out) return 1;

				/* Read DataN */
				*data_array = I2C_ReceiveData();

				/* Reset the number of bytes to be read by master */
				NumByteToRead = 0;
			}
		}
	}
	I2C_AcknowledgeConfig(I2C_ACK_CURR);
	return 0;
}

void GetTimeDate(void)
{
  i2c_mem_read_arr(0xD0, 0x00, receive_data, 7);
  Time.Seconds = BCD_TO_DEC(receive_data[0]);
  Time.Minutes = BCD_TO_DEC(receive_data[1]);
  Time.Hours = BCD_TO_DEC(receive_data[2]);
  Date.Day = BCD_TO_DEC(receive_data[3]);
  Date.Date = BCD_TO_DEC(receive_data[4]);
  Date.Month = BCD_TO_DEC(receive_data[5]);
  Date.Year = BCD_TO_DEC(receive_data[6]);
  
}
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
  if((MenuTimer)&&(MenuTimer != 0xFFFF)) MenuTimer++;
  if (TimingDelay)
  {
    TimingDelay--;
  }
  TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

INTERRUPT_HANDLER(IRQ_Handled_ADC1,22)
{
  static uint16_t ADC_Value;
  ADC_Value = ADC1_GetConversionValue();
  if((ADC_Value > 700) && (ADC_Value < 800))
  {
    Events.Select = 1;
    ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
    //TIM2_Cmd(DISABLE);
  }
  else 
    if((ADC_Value > 450) && (ADC_Value < 500))
    {
      Events.Left = 1;
      ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
      //TIM2_Cmd(ENABLE);
    }
    else 
      if((ADC_Value > 270) && (ADC_Value < 350))
      {
        Events.Down = 1;
        ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
      }
      else 
          if((ADC_Value > 100) && (ADC_Value < 200))
          {
            Events.Up = 1;
            ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
          }
          else 
              if((ADC_Value > 0) && (ADC_Value < 50))
              {
                Events.Right = 1;
                ADC1_ITConfig(ADC1_IT_EOCIE ,DISABLE);
              }
  ADC1_ClearITPendingBit(ADC1_IT_EOC);
}
