#ifndef DMA_UART_H
#define DMA_UART_H

#include "main.h"

// Sets up USART2 with DMA for both TX and RX.
// PA2 = TX, PA3 = RX, AF7 mapping, 9600 baud.
UART_Status DMAUART_Init(void);

// Starts a DMA transmit to send Size bytes.
// Returns UART_OK if started successfully.
UART_Status DMAUART_Transmit(uint8_t *pData, uint16_t Size);

// Starts a DMA receive to get Size bytes.
// Returns UART_OK if started successfully.
UART_Status DMAUART_Receive(uint8_t *pData, uint16_t Size);

// Waits for TX DMA to complete or time out.
UART_Status DMAUART_WaitTX(void);

// Waits for RX DMA to complete or time out.
UART_Status DMAUART_WaitRX(void);

#endif
