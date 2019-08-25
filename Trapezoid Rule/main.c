#include "stm8s.h"
#include "stdio.h"
#include "stdbool.h"
#include "math.h"

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

uint16_t int_sqrt(uint16_t a);
float UseRectangleRule(float Xmin, float Xmax, int16_t NumIntervals);
float f(float x);
float y;
int16_t Numerator;
int16_t Denominator;
int16_t R;
int main( void )
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  UART1_DeInit();
  // 115200 8N1
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  UART1_Cmd(ENABLE); 
  y = UseRectangleRule(0, 5, 100);
  R = y * 1000;
  Numerator = (R / 1000) % 1000;
  Denominator = R % 1000;
  printf("y = %d.%d", Numerator, Denominator);
}
float f(float x)
{
  return (1 + x + sin(2*x));
}
float UseRectangleRule(float Xmin, float Xmax, int16_t NumIntervals)
{
  float dx = (Xmax - Xmin) / NumIntervals;
  float TotalArea = 0;
  float x = Xmin;
  for(uint8_t i = 0; i < NumIntervals - 1; i++)
  {
    TotalArea = TotalArea + dx * (f(x) + f(x + dx)) / 2;
    x += dx; 
  }
  return TotalArea;
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
