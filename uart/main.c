#include "stm8s.h"
#include <stdlib.h>

#define RX_BUFFER_SIZE 5

uint8_t RX_Buffer[RX_BUFFER_SIZE];
uint8_t Index = 0;
uint8_t FLAG = 0;
uint16_t Number;
void Delay(void);


int main( void )
{ 
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  UART1_DeInit();
  // 115200 8N1
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  UART1_ITConfig( UART1_IT_RXNE_OR, ENABLE);
  enableInterrupts();
  UART1_Cmd(ENABLE);
  while(1)
  {  
    if(FLAG == 1)
    {
      Number = atoi((char *)RX_Buffer);
      FLAG = 0;
    }
  }
}

void Delay(void)
{
   for(uint16_t i=0;i<60000;i++)
   {
   
   }
}


INTERRUPT_HANDLER( UART1_RX_IRQHandler, 18)
{
  
if(UART1_GetFlagStatus(UART1_FLAG_RXNE))
{  
  RX_Buffer[Index] = UART1_ReceiveData8();
  Index++;
  if(Index == RX_BUFFER_SIZE)
  {
    Index = 0;
    FLAG = 1;
    Number = 0;
  }
}
}



#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  while (1)
  {
    
  }
}
#endif