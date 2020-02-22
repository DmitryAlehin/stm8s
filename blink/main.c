#include "stm8s.h"

void Delay(uint32_t delay);
void Delay(uint32_t delay);
uint32_t TimingDelay = 0;

int main( void )
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_OUT_OD_HIZ_SLOW);
  TIM4_TimeBaseInit(TIM4_PRESCALER_128, 124);
  TIM4_ClearFlag(TIM4_FLAG_UPDATE);
  TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
  enableInterrupts();
  TIM4_Cmd(ENABLE);
  while(1)
  {
   GPIO_WriteHigh(GPIOB, GPIO_PIN_5);
   Delay(100);
   GPIO_WriteLow(GPIOB, GPIO_PIN_5);
   Delay(100);
 }
}

INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
  if (TimingDelay)
  {
    TimingDelay--;
  }
  TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

void Delay(uint32_t delay)
{
  TimingDelay = delay;

  while (TimingDelay);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  while (1)
  {
    
  }
}
#endif