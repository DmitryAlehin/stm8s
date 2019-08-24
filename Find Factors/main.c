#include "stm8s.h"
#include "stdio.h"
#define NUMBER 15000

#ifdef _RAISONANCE_
#define PUTCHAR_PROTOTYPE int putchar (char c)
#define GETCHAR_PROTOTYPE int getchar (void)
#elif defined (_COSMIC_)
#define PUTCHAR_PROTOTYPE char putchar (char c)
#define GETCHAR_PROTOTYPE char getchar (void)
#else /* _IAR_ */
#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)
#endif /* _RAISONANCE_ */

void FindFactors(int16_t Number);
int main( void )
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  UART1_DeInit();
  // 115200 8N1
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  UART1_Cmd(ENABLE);
  FindFactors(NUMBER);
}

void FindFactors(int16_t Number)
{
  int16_t factor = 2;
  uint8_t NumberOfFactors= 0;
  while(factor < Number)
  {
    while(Number % factor == 0)
    {
      NumberOfFactors++;
      printf("Factor%d:%d\r\n", NumberOfFactors, factor);
      Number = Number / factor;
    }
    factor++;
  }
  if(Number > 1)
  {
    NumberOfFactors++;
    printf("Factor%d:%d\r\n", NumberOfFactors, Number);
  }
}

PUTCHAR_PROTOTYPE
{
  /* Write a character to the UART1 */
  UART1_SendData8(c);
  /* Loop until the end of transmission */
  while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);

  return (c);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  while (1)
  {
    
  }
}
#endif