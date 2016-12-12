#ifndef __UART_H
#define	__UART_H

#include "sys.h"

void USART1_Config(void);
void USART2_Config(void);
void uart1_send(uint8_t * dat,uint16_t len);

int fputc(int ch, FILE *f);
int fgetc(FILE *f);

#endif /* __USART1_H */
