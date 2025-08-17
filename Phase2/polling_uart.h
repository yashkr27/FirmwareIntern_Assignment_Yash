#ifndef POLLING_UART_H
#define POLLING_UART_H

#include "main.h"

// Initializes USART2 for polling (blocking) communication.
// 9600 baud, 8 data bits, 1 stop bit, no parity.
// PA2 = TX, PA3 = RX with AF7 mapping, RX line has a pull-up.
UART_Status PollingUART_Init(void);

// Sends data over USART2 using polling.
// Returns UART_OK if everything went fine, or an error/timeout status.
UART_Status PollingUART_Transmit(uint8_t *pData, uint16_t Size);

// Receives data over USART2 using polling.
// Returns UART_OK if everything went fine, or an error/timeout status.
UART_Status PollingUART_Receive(uint8_t *pData, uint16_t Size);

#endif
