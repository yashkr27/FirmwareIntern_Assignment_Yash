#ifndef INTERRUPT_UART_H
#define INTERRUPT_UART_H

#include "main.h"

void InterruptUART_Init(void);
UART_Status InterruptUART_Transmit(uint8_t *pData, uint16_t Size);
UART_Status InterruptUART_GetStatus(void);
uint8_t* InterruptUART_GetRxBuffer(void);

#endif /* INTERRUPT_UART_H */
