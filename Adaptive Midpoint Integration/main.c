#include "stm8s.h"
#include "stdio.h"
#include "stdbool.h"
#include "math.h"
#include "stdlib.h"

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
float IntegrateAdaptiveMidpoint(float Xmin, float Xmax, int16_t NumIntervals, float MaxSliceError);
float SliceArea(float x1, float x2, float MaxSliceError);
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
  y = IntegrateAdaptiveMidpoint(0, 5, 100, 1);
  R = y * 1000;
  Numerator = (R / 1000) % 1000;
  Denominator = R % 1000;
  printf("y = %d.%d", Numerator, Denominator);
}

float f(float x)
{
  return (1 + x + sin(2*x));
}

float IntegrateAdaptiveMidpoint(float Xmin, float Xmax, int16_t NumIntervals, float MaxSliceError)
{
  float dx = (Xmax - Xmin) / NumIntervals;
  double Total = 0;
  float TotalArea = 0;
  float x = Xmin;
  for(uint8_t i = 0; i < NumIntervals - 1; i++)
  {
    TotalArea = TotalArea + SliceArea(x, x +dx, MaxSliceError);
    x += dx; 
  }
  return TotalArea;
}

float SliceArea(float x1, float x2, float MaxSliceError)
{
  float y1 = f(x1);
  float y2 = f(x2);
  float xm = (x1 + x2) / 2;
  float ym = f(xm);
  float Area12 = (x2 - x1) * (y1 + y2) / 2.0;
  float Area1m = (xm - x1) * (y1 + ym) / 2.0;
  float Aream2 = (x2 - xm) * (ym + y2) / 2.0;
  float Area1m2 = Area1m + Aream2;
  float Error = (Area1m2 - Area12) / Area12;
  if(abs(Error) < MaxSliceError)
  {
    return Area1m2;
  }
  return SliceArea(x1, xm, MaxSliceError) + SliceArea(xm, x2, MaxSliceError);
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
