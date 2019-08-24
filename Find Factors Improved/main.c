#include "stm8s.h"
#include "stdio.h"
#define NUMBER 127

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
uint16_t root1(uint16_t a);
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
  volatile  uint8_t NumberOfFactors = 0;
  int16_t MaxFactor;
  int16_t Factor;
  while(Number % 2 == 0)
  {
    NumberOfFactors++;
    printf("Factor%d:%d\r\n", NumberOfFactors, 2);
    Number = Number / 2;
  }
  Factor = 3;
  MaxFactor = root1(Number);
  while(Factor <= MaxFactor)
  {
    while(Number % Factor == 0)
    {
      NumberOfFactors++;
      printf("Factor%d:%d\r\n", NumberOfFactors, Factor);
      Number /= Factor;
      MaxFactor = root1(Number);
    }
    Factor += 2;
  }
  if(Number > 1)
  {
    NumberOfFactors++;
    printf("Factor%d:%d\r\n", NumberOfFactors, Number);
  }
  NumberOfFactors = 0;
}

uint16_t root1(uint16_t a)
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