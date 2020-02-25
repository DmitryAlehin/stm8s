#include "stm8s.h"
#include "string.h"

#define E GPIOC, GPIO_PIN_3
#define RS GPIOC, GPIO_PIN_4
#define D7 GPIOD, GPIO_PIN_1
#define D6 GPIOC, GPIO_PIN_7
#define D5 GPIOC, GPIO_PIN_6
#define D4 GPIOC, GPIO_PIN_5
#define ALARM_INT GPIOD, GPIO_PIN_3

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

#define Flash_StartAddress      0x2000 - 8

typedef struct
{
  unsigned Select       : 1;
  unsigned Left         : 1;
  unsigned Right        : 1;
  unsigned Up           : 1;
  unsigned Down         : 1;
  unsigned Unpress      : 1;    
  unsigned Ready        : 1;
  unsigned LCD_Update   : 1;
}Events_t;

typedef struct
{
  uint8_t Hours;
  uint8_t Minutes;
  unsigned Enable       : 1;
  unsigned Interrupt    : 1;
  unsigned AllDayMode   : 1;
  unsigned WorkDayMode  : 1;
  unsigned Reserved     : 4;  
}Alarm_t;

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

typedef enum
{
  SET_DAY = 0,
  SET_DATE,
  SET_MONTH,
  SET_YEAR,
  SET_HOUR,
  SET_MINUTES,
  NOP
}Time_Setting_t;

typedef enum
{  
  ALARM_SET_ENABLE = 0,
  ALARM_SET_MODE,
  ALARM_SET_HOUR,
  ALARM_SET_MINUTES,
  ALARM_NOP
}Alarm_Setting_t;

Events_t Events;
Time_t Time;
Date_t Date;
Menu_States_t Menu_State;
Time_Setting_t Time_Setting;
Alarm_Setting_t Alarm_Setting;
Alarm_t Alarm1;
Alarm_t Alarm2;
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
uint8_t i2c_mem_write_arr(uint16_t device_address, uint16_t mem_address_start , uint8_t* data_array, uint16_t NumByteToWrite);
void GetTimeDate(void);
void SetTimeDate(void);
void WriteDataToFlash(uint8_t* pData, uint8_t* addr, uint8_t count);
volatile static uint32_t TimingDelay = 0;
static uint16_t MenuTimer;
static uint8_t receive_data[7];
static uint8_t old_data[7];
static uint8_t transmit_data[7];
static uint8_t ButtonDelay;
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
  ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_3, ADC1_PRESSEL_FCPU_D18,
          ADC1_EXTTRIG_TIM,DISABLE, ADC1_ALIGN_RIGHT,  ADC1_SCHMITTTRIG_CHANNEL3, DISABLE);
  
  

  
  enableInterrupts();
  TIM4_Cmd(ENABLE);
  
  Delay(150);
  LCD_Init();
  LCD_Clear();
  LCD_SetPos(0, 0);
  LCD_String(" ALARM  PROJECT");
  LCD_SetPos(0, 1);
  LCD_String("v0.1 by Dimka :)");  
  
  MenuTimer = 0xFFFF;
  Menu_State = TIME_MENU_MAIN;
  Time_Setting = NOP;
  Events.Ready = 1;
  Events.LCD_Update = 1;
  ButtonDelay = 0xFF;
  //Delay(300);
  GetTimeDate();
  ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
  ADC1_StartConversion();
  //LCD_Clear();
  while(1)
  { 
    if(ButtonDelay == 0xFF)Events.Ready = 1;
    
    switch(Menu_State)
    {
      case TIME_MENU_MAIN:
        
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();          
          Menu_State = ALARM_1_MAIN;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          Events.LCD_Update = 1;
        }
        if((Events.Select) & (Events.Unpress))
        {
          Clear_Events();
          LCD_Clear();
          Menu_State = TIME_SETTING;
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
        }
        if(MenuTimer == 0xFFFF)
        {          
          MenuTimer = 64535;          
          GetTimeDate();
          if(memcmp(&receive_data[1], &old_data[1], 6) != 0)
          {
            memcpy(old_data, receive_data, 7);
            Events.LCD_Update = 1;
          }
          
        }
        break;
      case ALARM_1_MAIN:
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = TIME_MENU_MAIN;
          Events.LCD_Update = 1;
        }
        if((Events.Select) & (Events.Unpress))
        {
          Clear_Events();
          LCD_Clear();
          Menu_State = ALARM_1_SETTING;
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
        }
        break;
      case ALARM_2_MAIN:
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();          
          Menu_State = TIME_MENU_MAIN;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_1_MAIN;
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
        }
        break;
    case TIME_SETTING:
      if((Events.Select) & (Events.Unpress))
      {
        Clear_Events();
       //Events.Ready = 1;        
        SetTimeDate();
        Menu_State = TIME_MENU_MAIN;
        Time_Setting = NOP;
        Events.LCD_Update = 1;        
      }
      switch(Time_Setting)
      {
      case SET_DAY:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Date.Day++;
          if(Date.Day >= 8)
          {
            Date.Day = 1;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Date.Day--;
          if(Date.Day <= 0)
          {
            Date.Day = 7;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DATE;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MINUTES;
          Events.LCD_Update = 1;
        }
        break;
      case SET_DATE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Date.Date++;
          if((Date.Month == 1) || (Date.Month == 3) || (Date.Month == 5) || (Date.Month == 7) || (Date.Month == 8) || (Date.Month == 10) || (Date.Month == 12))
          {
            if(Date.Date >= 32)
            {
              Date.Date = 1;
            }
          }
          else            
            if((Date.Month == 4) || (Date.Month == 6) || (Date.Month == 9) || (Date.Month == 11))
            {
              if(Date.Date >= 31)
              {
                Date.Date = 1;
              }
            }
            else
              if(Date.Month == 2)
              {
                if((Date.Year % 4) == 0)
                {
                  if(Date.Date >= 30)
                  {
                    Date.Date = 1;
                  }
                }
                else
                {
                  if(Date.Date >= 29)
                  {
                    Date.Date = 1;
                  }
                }
              }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Date.Date--;
          if((Date.Month == 1) || (Date.Month == 3) || (Date.Month == 5) || (Date.Month == 7) || (Date.Month == 8) || (Date.Month == 10) || (Date.Month == 12))
          {
            if(Date.Date <= 0)
            {
              Date.Date = 31;
            }
          }
          else            
            if((Date.Month == 4) || (Date.Month == 6) || (Date.Month == 9) || (Date.Month == 11))
            {
              if(Date.Date <= 0)
              {
                Date.Date = 30;
              }
            }
            else
              if(Date.Month == 2)
              {
                if((Date.Year % 4) == 0)
                {
                  if(Date.Date <= 0)
                  {
                    Date.Date = 29;
                  }
                }
                else
                {
                  if(Date.Date <= 0)
                  {
                    Date.Date = 28;
                  }
                }
              }
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MONTH;
          Events.LCD_Update = 1;
        }        
        break;
      case SET_MONTH:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Date.Month++;
          if(Date.Month >= 13)
          {
            Date.Month = 1;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Date.Month--;
          if(Date.Month <= 0)
          {
            Date.Month = 12;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_YEAR;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DATE;
          Events.LCD_Update = 1;
        }
        break;
      case SET_YEAR:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Date.Year++;
          if(Date.Year >= 100)
          {
            Date.Year = 0;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Date.Year--;
          if(/*(Date.Year < 0) || */(Date.Year == 255))
          {
            Date.Year = 99;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_HOUR;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MONTH;
          Events.LCD_Update = 1;
        }
        break;
      case SET_HOUR:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Time.Hours++;
          if(Time.Hours > 24)
          {
            Time.Hours = 0;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Time.Hours--;
          if(/*(Date.Year < 0) || */(Date.Year == 255))
          {
            Time.Hours = 24;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MINUTES;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_YEAR;
          Events.LCD_Update = 1;
        }
        break;
      case SET_MINUTES:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Time.Minutes++;
          if(Time.Minutes >= 60)
          {
            Time.Minutes = 0;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Time.Minutes--;
          if(/*(Date.Year < 0) || */(Time.Minutes == 255))
          {
            Time.Minutes = 59;
          }
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_HOUR;
          Events.LCD_Update = 1;
        }
        break;
      case NOP:
        break;
      }
      break;
    case ALARM_1_SETTING:
      if((Events.Select) & (Events.Unpress))
      {
        Clear_Events();
        //enable alarm 1
        Menu_State = ALARM_1_MAIN;
        Alarm_Setting = ALARM_NOP;
        Events.LCD_Update = 1;        
      }
      switch(Alarm_Setting)
      {
      case ALARM_NOP:
        break;
      case ALARM_SET_ENABLE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm1.Enable = 1;
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm1.Enable = 0;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MINUTES;
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MODE;
          Events.LCD_Update = 1;
        }
        break;
      case ALARM_SET_MODE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm1.AllDayMode = 1;
          Alarm1.WorkDayMode = 0;
          Events.LCD_Update = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm1.AllDayMode = 0;
          Alarm1.WorkDayMode = 1;
          Events.LCD_Update = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_HOUR;
          Events.LCD_Update = 1;
        }
        break;
      case ALARM_SET_HOUR:
        break;
      case ALARM_SET_MINUTES:
        break;
      }
      break;
    case ALARM_2_SETTING:
      break;
    }
    
    if((Events.LCD_Update))
    {
      LCD_Clear();
      Events.LCD_Update = 0;
      if((Menu_State == TIME_MENU_MAIN) || (Menu_State == TIME_SETTING))
      {        
        LCD_SetPos(0, 0);
        if((Time_Setting == SET_DATE) || (Time_Setting == SET_MONTH) || (Time_Setting == SET_YEAR) || (Time_Setting == NOP))
        {   
          
          LCD_SendChar((char) ((Date.Date/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Date%10) + 0x30);
          LCD_SendChar('.');
          
          LCD_SendChar((char) ((Date.Month/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Month%10) + 0x30);
          LCD_SendChar('.');
          
          LCD_SendChar((char) ((Date.Year/10)%10) + 0x30);
          LCD_SendChar((char) (Date.Year%10) + 0x30);
        }
        LCD_String("   ");
        if((Time_Setting == SET_HOUR) || (Time_Setting == SET_MINUTES) || (Time_Setting == NOP))
        {
          LCD_SendChar((char) ((Time.Hours/10)%10) + 0x30);
          LCD_SendChar((char) (Time.Hours%10) + 0x30);
          LCD_SendChar(':');
          
          LCD_SendChar((char) ((Time.Minutes/10)%10) + 0x30);
          LCD_SendChar((char) (Time.Minutes%10) + 0x30);
        }
        LCD_SetPos(1, 1);
        if((Time_Setting == SET_DAY) || (Time_Setting == NOP))
        {
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
        switch(Time_Setting)
        {
        case SET_DAY:
          LCD_SetPos(0, 0);
          LCD_String("Set day");
          break;
        case SET_DATE:
          LCD_SetPos(1, 1);
          LCD_String("Set date");
          break;
        case SET_MONTH:
          LCD_SetPos(1, 1);
          LCD_String("Set month");
          break;
        case SET_YEAR:
          LCD_SetPos(1, 1);
          LCD_String("Set year");
          break;
        case SET_HOUR:
          LCD_SetPos(1, 1);
          LCD_String("Set hour");
          break;
        case SET_MINUTES:
          LCD_SetPos(1, 1);
          LCD_String("Set minutes");
          break;
        }
      }
      else
        if((Menu_State == ALARM_1_SETTING) || (Menu_State == ALARM_2_SETTING) || (Menu_State == ALARM_1_MAIN) || (Menu_State == ALARM_2_MAIN))
        {
          switch(Menu_State)
          {
          case ALARM_1_MAIN:            
            LCD_SetPos(0, 0);
            LCD_String("    ALARM 1");
            LCD_SetPos(1, 1);  
            LCD_SendChar((char) ((Alarm1.Hours/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm1.Hours%10) + 0x30);
            LCD_SendChar(':');
            
            LCD_SendChar((char) ((Alarm1.Minutes/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm1.Minutes%10) + 0x30);
            LCD_String("   ");
            if(Alarm1.Enable)
            {                          
              LCD_String("ON");
            }
            else
            {
              LCD_String("OFF");
            }
            break;
          case ALARM_2_MAIN:
            LCD_SetPos(0, 0);
            LCD_String("    ALARM 2");
            LCD_SetPos(1, 1);  
            LCD_SendChar((char) ((Alarm2.Hours/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm2.Hours%10) + 0x30);
            LCD_SendChar(':');
            
            LCD_SendChar((char) ((Alarm2.Minutes/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm2.Minutes%10) + 0x30);
            LCD_String("   ");
            if(Alarm2.Enable)
            {                          
              LCD_String("ON");
            }
            else
            {
              LCD_String("OFF");
            }
            break;   
          case ALARM_1_SETTING:
            switch(Alarm_Setting)
            {
            case ALARM_SET_ENABLE: 
              LCD_SetPos(0, 0);
              LCD_String("ALARM 1 enable");
              LCD_SetPos(1, 1);  
              LCD_SendChar((char) ((Alarm1.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm1.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm1.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm1.Minutes%10) + 0x30);
              LCD_String("   ");
              if(Alarm1.Enable)
              {                          
                LCD_String("ON");
              }
              else
              {
                LCD_String("OFF");
              }
              break;
            case ALARM_SET_MODE:
              LCD_SetPos(0, 0);
              LCD_String("ALARM 1 mode");
              LCD_SetPos(1, 1);  
              if(Alarm1.AllDayMode)
              {
                LCD_String("All day");
              }
              else
                if(Alarm1.WorkDayMode)
                {
                  LCD_String("Work days only");
                }
                break;
            }
            break;
          }
        }
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
    Delay(1);
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
  uint16_t i;
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
  Events.Unpress = 0;
}

void Init_GPIO(void)
{
  GPIO_Init(D4, GPIO_MODE_OUT_PP_LOW_SLOW); 
  GPIO_Init(D5, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(D6, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(D7, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(RS, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(E, GPIO_MODE_OUT_PP_LOW_SLOW);
  GPIO_Init(ALARM_INT, GPIO_MODE_IN_PU_IT);
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
	
	I2C_AcknowledgeConfig(I2C_ACK_CURR);
	return 0;
}

uint8_t i2c_mem_write_arr(uint16_t device_address, uint16_t mem_address_start , uint8_t* data_array, uint16_t NumByteToWrite)
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

	for(uint16_t i=0;i<NumByteToWrite;i++)
	{
		I2C_SendData(*(data_array+i));
		while((!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))&& (--time_out));
		if(!time_out) return 1;
	}

	I2C_GenerateSTOP(ENABLE);
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

void SetTimeDate(void)
{
  transmit_data[0] = DEC_TO_BCD(Time.Seconds);
  transmit_data[1] = DEC_TO_BCD(Time.Minutes);
  transmit_data[2] = DEC_TO_BCD(Time.Hours);
  transmit_data[3] = DEC_TO_BCD(Date.Day);
  transmit_data[4] = DEC_TO_BCD(Date.Date);
  transmit_data[5] = DEC_TO_BCD(Date.Month);
  transmit_data[6] = DEC_TO_BCD(Date.Year);
  i2c_mem_write_arr(0xD0, 0x00, transmit_data, 7);
}
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
  if((MenuTimer)&&(MenuTimer != 0xFFFF)) MenuTimer++;
  if((ButtonDelay)&&(ButtonDelay != 0xFF)) ButtonDelay++;
  
  if (TimingDelay)
  {
    TimingDelay--;
  }
  TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
  if(!GPIO_ReadInputPin(ALARM_INT))
  {
    
  }
}

INTERRUPT_HANDLER(IRQ_Handled_ADC1,22)
{
  static uint16_t ADC_Value;
  ADC_Value = ADC1_GetConversionValue(); 
if(Events.Ready) 
{
  
  if((ADC_Value > 800) && (ButtonDelay == 0xFF))
  {
    if(Events.Select || Events.Left || Events.Down || Events.Up || Events.Right)
    {
      Events.Unpress = 1;
      Events.Ready = 0;
    }
  }
  else
    if((ADC_Value > 600) && (ADC_Value < 800))
    {
      Events.Select = 1;
      ButtonDelay =  0xFF - 200;
      //Events.Ready = 0;
    }
    else 
      if((ADC_Value > 400) && (ADC_Value < 500))
      {
        Events.Left = 1;
        ButtonDelay =  0xFF - 200;
        //Events.Ready = 0;
      }
      else 
        if((ADC_Value > 270) && (ADC_Value < 350))
        {
          Events.Down = 1;
          ButtonDelay =  0xFF - 200;
          //Events.Ready = 0;
        }
        else 
            if((ADC_Value > 100) && (ADC_Value < 200))
            {
              Events.Up = 1;
              ButtonDelay =  0xFF - 200;
              //Events.Ready = 0;
            }
            else 
                if((ADC_Value > 0) && (ADC_Value < 50))
                {
                  Events.Right = 1;
                  ButtonDelay =  0xFF - 200;
                  //Events.Ready = 0;
                }
}
  ADC1_ClearITPendingBit(ADC1_IT_EOC);
}

void WriteDataToFlash(uint8_t* pData, uint8_t* addr, uint8_t count)
{	uint8_t i;
	FLASH_DeInit();
	FLASH_Unlock(FLASH_MEMTYPE_PROG);
	if(FLASH->IAPSR & FLASH_IAPSR_PUL)
	{	for (i = 0; i < count; i++)
		{	*(addr)=(uint8_t)pData[i];
			addr++;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_PROG);
		}
	}
	FLASH_Lock(FLASH_MEMTYPE_PROG);
}