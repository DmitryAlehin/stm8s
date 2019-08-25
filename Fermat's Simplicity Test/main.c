#include "stm8s.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "math.h"
#define MAX_NUMBER 150L

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
bool IsPrime(int16_t p, int16_t MaxTests);
uint16_t int_sqrt(uint16_t a);
int main( void )
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  UART1_DeInit();
  // 115200 8N1
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  UART1_Cmd(ENABLE);
  if(IsPrime(12, 10))
  {
    printf("The number is prime");
  }
  else
  {
    printf("The number is not prime");
  }
}

bool IsPrime(int16_t p, int16_t MaxTests)
{
  int16_t n;
  for(uint8_t i = 1; i <= MaxTests; i++)
  {
    n = 1 + rand() % p;
    uint32_t k = pow(n, p - 1);
    if((k % p) == 1)
    {
      return true;
    }
  }
  return false;
}

uint16_t int_sqrt(uint16_t a)
{
   uint16_t x;
   x = (a/0x3f + 0x3f)>>1;
   x = (a/x + x)>>1;
   x = (a/x + x)>>1;
   return(x); 
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
