#include "stm8s.h"
#include "stdio.h"

#define NUMBER_ONE 4851
#define NUMBER_TWO 3003

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

int16_t GCD(int16_t Number1, int16_t Number2);
int16_t remainder = 0;
int16_t Result = 0;
int main( void )
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  UART1_DeInit();
  // 115200 8N1
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  UART1_Cmd(ENABLE);
  Result = GCD(NUMBER_ONE, NUMBER_TWO);
  printf("Наибольший общий делитель чисел %d и %d: %d\r\n", NUMBER_ONE, NUMBER_TWO, Result);
}

int16_t GCD(int16_t Number1, int16_t Number2)
{
  while(Number2)
  {
    remainder = Number1 % Number2;
    Number1 = Number2;
    Number2 = remainder;
  }
  return Number1;
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