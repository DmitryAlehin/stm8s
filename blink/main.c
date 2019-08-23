#include "stm8s.h"

void Delay(void);

int main( void )
{
  GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_OUT_OD_HIZ_SLOW);
  while(1)
  {
   GPIO_WriteHigh(GPIOB, GPIO_PIN_5);
   Delay();
   GPIO_WriteLow(GPIOB, GPIO_PIN_5);
   Delay();
 }
}
void Delay(void)
{
   for(uint16_t i=0;i<60000;i++)
   {
   
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