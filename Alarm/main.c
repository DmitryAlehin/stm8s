#include "stm8s.h"
#include "string.h"

#define E GPIOC, GPIO_PIN_3
#define RS GPIOC, GPIO_PIN_4
#define D7 GPIOD, GPIO_PIN_1
#define D6 GPIOC, GPIO_PIN_7
#define D5 GPIOC, GPIO_PIN_6
#define D4 GPIOC, GPIO_PIN_5
#define ALARM_INT GPIOD, GPIO_PIN_3
#define ALARM_OFF GPIOD, GPIO_PIN_5

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

#define Flash_StartAddress      0x4000

#define ONE_TIME_ALARM          0
#define ALL_DAY_MODE            1
#define WORK_DAY_MODE           2
#define ERROR                   0
#define ALARM_1                 1
#define ALARM_2                 2

#define DS3231_ADDRESS          0xD0
#define TIME_OFFSET             0x00
#define ALARM_1_ADDRESS_OFFSET  0x07         
#define ALARM_2_ADDRESS_OFFSET  0x0B
#define CTRL_REG_OFFSET         0x0E        
#define STATUS_REG_OFFSET       0x0F
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
  unsigned Mode         : 2;
  unsigned Reserved     : 4;  
}Alarm_t;

typedef struct
{
  Alarm_t Alarm1;
  Alarm_t Alarm2;
}
Alarms_t;

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
Alarms_t Alarm;
volatile static uint32_t TimingDelay = 0;
static uint16_t MenuTimer;
static uint8_t receive_data[7];
static uint8_t old_data[7];
static uint8_t transmit_data[7];
static uint8_t ButtonDelay;
static uint8_t BRNG_State;
const uint8_t BRNG_Tune_arr[10] = {254, 100, 50, 70, 100, 50, 70, 100, 50, 70};
static uint8_t BRNG_Timer;

void LCD_Init(void);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_SetPos(uint8_t x, uint8_t y);
void LCD_Data(uint8_t dt);
void LCD_String(char* st);
void LCD_SendChar(char ch);
void LCD_Clear(void);
void GetAlarmSettings(Alarms_t * Alarm);
void Delay(uint32_t delay);
void delay(void);
void Clear_Events(void);
void Init_GPIO(void);
void Init_Timers(void);
void GetTimeDate(void);
void SetTimeDate(void);
void WriteDataToFlash(uint8_t* pData, uint8_t* addr, uint8_t count);
uint8_t BCD_TO_DEC(uint8_t data);
uint8_t DEC_TO_BCD(uint8_t data);
uint8_t i2c_mem_read_arr(uint16_t device_address, uint16_t mem_address_start, uint8_t* data_array, uint16_t NumByteToRead);
uint8_t i2c_mem_write_arr(uint16_t device_address, uint16_t mem_address_start , uint8_t* data_array, uint16_t NumByteToWrite);
void AlarmSet(uint8_t AlarmSt);
void AlarmIntDisable(uint8_t AlarmEn);
void AlarmIntEnable(uint8_t AlarmEn);
void CheckAlarms(void);
void AlarmBRNG(void);

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
  ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_3, ADC1_PRESSEL_FCPU_D2, 
            ADC1_EXTTRIG_TIM,DISABLE, ADC1_ALIGN_LEFT,  ADC1_SCHMITTTRIG_CHANNEL3, DISABLE);
  
  enableInterrupts();
  TIM4_Cmd(ENABLE);
  
  Delay(150);
  LCD_Init();
  LCD_Clear();
  LCD_SetPos(0, 0);
  LCD_String(" ALARM  PROJECT");
  LCD_SetPos(0, 1);
  LCD_String("v0.1 by Dimka :)");  
  GetAlarmSettings(&Alarm);
  MenuTimer = 0xFFFF;
  Menu_State = TIME_MENU_MAIN;
  Time_Setting = NOP;
  Events.Ready = 1;
  Events.LCD_Update = 1;
  BRNG_State = 0;
  BRNG_Timer = 0;
  ButtonDelay = 0xFF;
  //Delay(300);
  GetTimeDate();
  ADC1_ITConfig(ADC1_IT_EOCIE ,ENABLE);
  ADC1_StartConversion();
  //LCD_Clear();
  while(1)
  { 
    //if(ButtonDelay == 0xFF)Events.Ready = 1;
    
    switch(Menu_State)
    {
      case TIME_MENU_MAIN:
        
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();          
          Menu_State = ALARM_1_MAIN;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Select) & (Events.Unpress))
        {
          Clear_Events();
          LCD_Clear();
          Menu_State = TIME_SETTING;
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }  
        if(MenuTimer == 0xFFFF)
        {          
          MenuTimer = 0xFFFF - 1000;          
          GetTimeDate();
          Events.LCD_Update = 1;
          /*
          if(memcmp(&receive_data[1], &old_data[1], 7) != 0)
          {            
            memcpy(&old_data[0], &receive_data[0], 7);
            Events.LCD_Update = 1;
          }
*/
        }
        break;
      case ALARM_1_MAIN:
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_2_MAIN;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = TIME_MENU_MAIN;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Select) & (Events.Unpress))
        {
          Clear_Events();
          LCD_Clear();
          Menu_State = ALARM_1_SETTING;
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_2_MAIN:
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();          
          Menu_State = TIME_MENU_MAIN;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Menu_State = ALARM_1_MAIN;
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Select) & (Events.Unpress))
        {
          Clear_Events();
          LCD_Clear();
          Menu_State = ALARM_2_SETTING;
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
        Events.Ready = 1;
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
          Events.Ready = 1;
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
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DATE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
          Events.Ready = 1;
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
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MONTH;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
          Events.Ready = 1;
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
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_YEAR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DATE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Date.Year--;
          if(Date.Year == 255)
          {
            Date.Year = 99;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_HOUR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MONTH;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case SET_HOUR:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Time.Hours++;
          if(Time.Hours > 23)
          {
            Time.Hours = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Time.Hours--;
          if(Time.Hours == 255)
          {
            Time.Hours = 23;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_YEAR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Time.Minutes--;
          if(Time.Minutes == 255)
          {
            Time.Minutes = 59;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_DAY;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Time_Setting = SET_HOUR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
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
        WriteDataToFlash((uint8_t *)&Alarm, (uint8_t *)Flash_StartAddress, sizeof(Alarms_t));
        AlarmSet(ALARM_1);
        if(Alarm.Alarm2.Enable)
        {
          AlarmIntEnable(ALARM_1);
        }
        else
        {
          AlarmIntDisable(ALARM_1);
        }
        //enable alarm 1
        Menu_State = ALARM_1_MAIN;
        Alarm_Setting = ALARM_NOP;
        Events.LCD_Update = 1; 
        Events.Ready = 1;
      }
      switch(Alarm_Setting)
      {
      case ALARM_NOP:
        break;
      case ALARM_SET_ENABLE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Enable = 1;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Enable = 0;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MODE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_MODE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Mode++;
          if(Alarm.Alarm1.Mode == 3)
          {
            Alarm.Alarm1.Mode = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Mode--;
          if(Alarm.Alarm1.Mode == 255)
          {
            Alarm.Alarm1.Mode = 3;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_HOUR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_HOUR:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Hours++;
          if(Alarm.Alarm1.Hours > 23)
          {
            Alarm.Alarm1.Hours = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Hours--;
          if(Alarm.Alarm1.Hours == 255)
          {
            Alarm.Alarm1.Hours = 23;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MODE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_MINUTES:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Minutes++;
          if(Alarm.Alarm1.Minutes >= 60)
          {
            Alarm.Alarm1.Minutes = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm1.Minutes--;
          if(Alarm.Alarm1.Minutes == 255)
          {
            Alarm.Alarm1.Minutes = 59;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_HOUR;
          Events.LCD_Update = 1;          
          Events.Ready = 1;
        }
        break;
      }
      break;
    case ALARM_2_SETTING:
      if((Events.Select) & (Events.Unpress))
      {
        Clear_Events();
        WriteDataToFlash((uint8_t *)&Alarm, (uint8_t *)Flash_StartAddress, sizeof(Alarms_t));
        AlarmSet(ALARM_2);
        if(Alarm.Alarm2.Enable)
        {
          AlarmIntEnable(ALARM_2);
        }
        else
        {
          AlarmIntDisable(ALARM_2);
        }
        //enable alarm 1
        Menu_State = ALARM_2_MAIN;
        Alarm_Setting = ALARM_NOP;
        Events.LCD_Update = 1;    
        Events.Ready = 1;
      }
      switch(Alarm_Setting)
      {
      case ALARM_NOP:
        break;
      case ALARM_SET_ENABLE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Enable = 1;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Enable = 0;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MODE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_MODE:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Mode++;
          if(Alarm.Alarm2.Mode == 3)
          {
            Alarm.Alarm2.Mode = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Mode--;
          if(Alarm.Alarm2.Mode == 255)
          {
            Alarm.Alarm2.Mode = 3;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_HOUR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_HOUR:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Hours++;
          if(Alarm.Alarm2.Hours > 23)
          {
            Alarm.Alarm2.Hours = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Hours--;
          if(Alarm.Alarm2.Hours == 255)
          {
            Alarm.Alarm2.Hours = 23;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MINUTES;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_MODE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      case ALARM_SET_MINUTES:
        if((Events.Up) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Minutes++;
          if(Alarm.Alarm2.Minutes >= 60)
          {
            Alarm.Alarm2.Minutes = 0;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Down) & (Events.Unpress))
        {
          Clear_Events();
          Alarm.Alarm2.Minutes--;
          if(Alarm.Alarm2.Minutes == 255)
          {
            Alarm.Alarm2.Minutes = 59;
          }
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Right) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_ENABLE;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        if((Events.Left) & (Events.Unpress))
        {
          Clear_Events();
          Alarm_Setting = ALARM_SET_HOUR;
          Events.LCD_Update = 1;
          Events.Ready = 1;
        }
        break;
      }
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
            LCD_SendChar((char) ((Alarm.Alarm1.Hours/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm.Alarm1.Hours%10) + 0x30);
            LCD_SendChar(':');
            
            LCD_SendChar((char) ((Alarm.Alarm1.Minutes/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm.Alarm1.Minutes%10) + 0x30);
            LCD_String("   ");
            if(Alarm.Alarm1.Enable)
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
            LCD_SendChar((char) ((Alarm.Alarm2.Hours/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm.Alarm2.Hours%10) + 0x30);
            LCD_SendChar(':');
            
            LCD_SendChar((char) ((Alarm.Alarm2.Minutes/10)%10) + 0x30);
            LCD_SendChar((char) (Alarm.Alarm2.Minutes%10) + 0x30);
            LCD_String("   ");
            if(Alarm.Alarm2.Enable)
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
              LCD_SendChar((char) ((Alarm.Alarm1.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm1.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Minutes%10) + 0x30);
              LCD_String("   ");
              if(Alarm.Alarm1.Enable)
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
              switch(Alarm.Alarm1.Mode)
              {
                case ONE_TIME_ALARM:
                  LCD_String("One time");
                  break;
                case ALL_DAY_MODE:
                  LCD_String("All day");
                  break;
                case WORK_DAY_MODE:
                  LCD_String("Work days only");
                  break;
              }
              break;
            case ALARM_SET_HOUR:
              LCD_SetPos(0, 0);
              LCD_String("Set hour");
              LCD_SetPos(1, 1); 
              LCD_SendChar((char) ((Alarm.Alarm1.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm1.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Minutes%10) + 0x30);
              break;
            case ALARM_SET_MINUTES:
              LCD_SetPos(0, 0);
              LCD_String("Set minutes");
              LCD_SetPos(1, 1);
              LCD_SendChar((char) ((Alarm.Alarm1.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm1.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm1.Minutes%10) + 0x30);
              break;
            }
            break;
          case ALARM_2_SETTING:
            switch(Alarm_Setting)
            {
            case ALARM_SET_ENABLE: 
              LCD_SetPos(0, 0);
              LCD_String("ALARM 2 enable");
              LCD_SetPos(1, 1);  
              LCD_SendChar((char) ((Alarm.Alarm2.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm2.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Minutes%10) + 0x30);
              LCD_String("   ");
              if(Alarm.Alarm2.Enable)
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
              LCD_String("ALARM 2 mode");
              LCD_SetPos(1, 1); 
              switch(Alarm.Alarm2.Mode)
              {
                case ONE_TIME_ALARM:
                  LCD_String("One time");
                  break;
                case ALL_DAY_MODE:
                  LCD_String("All day");
                  break;
                case WORK_DAY_MODE:
                  LCD_String("Work days only");
                  break;
              }
              break;
            case ALARM_SET_HOUR:
              LCD_SetPos(0, 0);
              LCD_String("Set hour");
              LCD_SetPos(1, 1); 
              LCD_SendChar((char) ((Alarm.Alarm2.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm2.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Minutes%10) + 0x30);
              break;
            case ALARM_SET_MINUTES:
              LCD_SetPos(0, 0);
              LCD_String("Set minutes");
              LCD_SetPos(1, 1);
              LCD_SendChar((char) ((Alarm.Alarm2.Hours/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Hours%10) + 0x30);
              LCD_SendChar(':');
              
              LCD_SendChar((char) ((Alarm.Alarm2.Minutes/10)%10) + 0x30);
              LCD_SendChar((char) (Alarm.Alarm2.Minutes%10) + 0x30);
              break;
            }
            break;
          }
        }
    }
    AlarmBRNG();
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
    LCD_WriteCmd(0x28);	
    Delay(1);
    LCD_WriteCmd(0x28);
    Delay(1);
    LCD_WriteCmd(0x0C);
    Delay(1);
    LCD_WriteCmd(0x01);
    Delay(2);
    LCD_WriteCmd(0x06);
    Delay(1);
    LCD_WriteCmd(0x02);
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
  GPIO_Init(ALARM_OFF, GPIO_MODE_IN_PU_IT);
  EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_FALL_ONLY);
  EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);
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
  i2c_mem_read_arr(DS3231_ADDRESS, TIME_OFFSET, receive_data, 7);
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
  i2c_mem_write_arr(DS3231_ADDRESS, TIME_OFFSET, transmit_data, 7);
}
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
  if((MenuTimer) && (MenuTimer != 0xFFFF)) MenuTimer++;
  if((ButtonDelay) && (ButtonDelay != 0xFF)) ButtonDelay++;
  if((BRNG_Timer) && (BRNG_Timer != 0xFF)) BRNG_Timer++;
  if (TimingDelay)
  {
    TimingDelay--;
  }
  TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
   static volatile uint8_t k;
  k = EXTI_GetExtIntSensitivity(EXTI_PORT_GPIOD);
  if((GPIO_ReadInputData(GPIOD) & GPIO_PIN_3) == 0x00)
  {
    CheckAlarms();
  }
  if((GPIO_ReadInputData(GPIOD) & GPIO_PIN_5) == 0x00)
  {
    if(Alarm.Alarm1.Interrupt)
    {
      Alarm.Alarm1.Interrupt = 0;
    }
    if(Alarm.Alarm2.Interrupt)
    {
      Alarm.Alarm2.Interrupt = 0;
    }
  }
  
}

INTERRUPT_HANDLER(IRQ_Handled_ADC1,22)
{
  static uint16_t ADC_Value;
  ADC_Value = ADC1_GetConversionValue(); 
  if(Events.Ready) 
  {
    
    if((ADC_Value > 45000) && (ButtonDelay == 0xFF))
    {
      if(Events.Select || Events.Left || Events.Down || Events.Up || Events.Right)
      {
        Events.Unpress = 1;
        Events.Ready = 0;
      }
    }
    else
      if((ADC_Value > 35000) && (ADC_Value < 45000))
      {
        Events.Select = 1;
        ButtonDelay =  0xFF - 20;
      }
      else 
        if((ADC_Value > 25000) && (ADC_Value < 35000))
        {
          Events.Left = 1;
          ButtonDelay =  0xFF - 20;
        }
        else 
          if((ADC_Value > 15000) && (ADC_Value < 25000))
          {
            Events.Down = 1;
            ButtonDelay =  0xFF - 20;
          }
          else 
              if((ADC_Value > 5000) && (ADC_Value < 15000))
              {
                Events.Up = 1;
                ButtonDelay =  0xFF - 20;
              }
              else 
                  if((ADC_Value >= 0) && (ADC_Value < 5000))
                  {
                    Events.Right = 1;
                    ButtonDelay =  0xFF - 20;
                  }
  }
  ADC1_ClearITPendingBit(ADC1_IT_EOC);
}

void WriteDataToFlash(uint8_t* pData, uint8_t* addr, uint8_t count)
{	uint8_t i;
	//FLASH_DeInit();
	FLASH_Unlock(FLASH_MEMTYPE_DATA);
        FLASH_Unlock(FLASH_MEMTYPE_PROG);
	if(FLASH->IAPSR & FLASH_IAPSR_PUL)
	{	for (i = 0; i < count; i++)
		{	*(addr)=(uint8_t)pData[i];
			addr++;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
		}
	}
	FLASH_Lock(FLASH_MEMTYPE_DATA);
        FLASH_Lock(FLASH_MEMTYPE_PROG);
}

void GetAlarmSettings(Alarms_t * Alarm)
{
  uint8_t* Addres;
  uint8_t* Byte;
  uint8_t i;
  Addres = (uint8_t*)(Flash_StartAddress);
  Byte = (uint8_t*)Alarm;
  
  for(i = 0; i < sizeof(Alarms_t); i++)
  {	*Byte = *Addres;
          Addres++;
          Byte++;
  }
}

void AlarmSet(uint8_t AlarmSt)
{
  uint8_t TxBuffer[5];
  uint8_t Offset;
  if(AlarmSt == ALARM_1)
  {
    //TxBuffer[0] &= 0x7F;
    TxBuffer[1] = DEC_TO_BCD(Alarm.Alarm1.Minutes);
    TxBuffer[2] = DEC_TO_BCD(Alarm.Alarm1.Hours);
    TxBuffer[3] &= 0x80;
    TxBuffer[1] &= 0x7F;
    TxBuffer[2] &= 0x7F;
    Offset = ALARM_1_ADDRESS_OFFSET;
  }
  else
    if(AlarmSt == ALARM_2)
    {
      TxBuffer[0] = DEC_TO_BCD(Alarm.Alarm2.Minutes);
      TxBuffer[1] = DEC_TO_BCD(Alarm.Alarm2.Hours);
      TxBuffer[2] &= 0x80;
      TxBuffer[0] &= 0x7F;
      TxBuffer[1] &= 0x7F;
      Offset = ALARM_2_ADDRESS_OFFSET;
    }else return;
  i2c_mem_write_arr(DS3231_ADDRESS, Offset, TxBuffer, 4);
}

void AlarmIntEnable(uint8_t AlarmEn)
{
  uint8_t CtrlByte = 0;
  if(AlarmEn == ALARM_1)
  {
    CtrlByte |= 0x05;
  }
  else
    if(AlarmEn == ALARM_2)
    {
      CtrlByte |= 0x06;
    }else return;
   i2c_mem_write_arr(DS3231_ADDRESS, CTRL_REG_OFFSET, &CtrlByte, 1);   
}

void AlarmIntDisable(uint8_t AlarmEn)
{
  uint8_t CtrlByte = 0;
  if(AlarmEn == ALARM_1)
  {
    CtrlByte &= ~0x05;
  }
  else
    if(AlarmEn == ALARM_2)
    {
      CtrlByte &= ~0x06;
    }else return;
   i2c_mem_write_arr(DS3231_ADDRESS, CTRL_REG_OFFSET, &CtrlByte, 1);   
}

void CheckAlarms(void)
{
  uint8_t StatusByte = 0;
  i2c_mem_read_arr(DS3231_ADDRESS, STATUS_REG_OFFSET, &StatusByte, 1);
  if(StatusByte & 0x01)
  {
    Alarm.Alarm1.Interrupt = 1;    
  }
  if(StatusByte & 0x02)
  {
    Alarm.Alarm2.Interrupt = 1;
  }
}

void AlarmBRNG(void)
{  
  uint8_t BRNG_RollTimer = 0;
  if((Alarm.Alarm1.Interrupt) || (Alarm.Alarm2.Interrupt))
  {    
    switch(BRNG_State)
    {
    case 0:
      TIM2_Cmd(ENABLE);
      BRNG_RollTimer = 0;
      BRNG_Timer = BRNG_Tune_arr[BRNG_RollTimer];
      BRNG_State++;
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      if(BRNG_Timer == 0xFF)
      {
        TIM2_Cmd(DISABLE);
        BRNG_RollTimer++;
        BRNG_Timer = BRNG_Tune_arr[BRNG_RollTimer];
        BRNG_State++;
      }
      break;
    case 2:
    case 4:
    case 6:
    case 8:
      if(BRNG_Timer == 0xFF)
      {
        TIM2_Cmd(ENABLE);
        BRNG_RollTimer++;
        BRNG_Timer = BRNG_Tune_arr[BRNG_RollTimer];
        BRNG_State++;
      }
      break;
    case 10:
      BRNG_State = 0;
      BRNG_Timer = 0;
      break;
    default:
      BRNG_State = 0;
      BRNG_Timer = 0;
      break;
    }
  }
}