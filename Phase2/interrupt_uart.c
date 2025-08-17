#include "interrupt_uart.h"

// --- Private Driver State ---
static volatile uint8_t *pTxData;
static volatile uint16_t txLen = 0;

#define RX_BUFFER_SIZE 50
static uint8_t rxBuffer[RX_BUFFER_SIZE];
static volatile uint16_t rxIndex = 0;

// We use this status variable as a lock
static volatile UART_Status uartStatus = UART_OK;

// Flags to track completion of both TX and RX
static volatile uint8_t txComplete = 0;
static volatile uint8_t rxComplete = 0;


void InterruptUART_Init(void) {
    // GPIO and UART configuration remains the same...
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);

    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos);

    USART2->CR1 &= ~USART_CR1_UE;
    USART2->CR1 &= ~USART_CR1_M;
    USART2->CR2 &= ~USART_CR2_STOP;
    USART2->BRR = 0x1117; // 9600 baud
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

    // --- Interrupt Configuration ---
    USART2->CR1 |= USART_CR1_RXNEIE; // Enable RX interrupt
    NVIC_EnableIRQ(USART2_IRQn);    // Enable USART2 interrupt in NVIC
    USART2->CR1 |= USART_CR1_UE;      // Enable USART
}

UART_Status InterruptUART_Transmit(uint8_t *pData, uint16_t Size) {
    // If a transfer is already in progress, return busy status
    if (uartStatus == UART_BUSY) {
        return UART_BUSY;
    }

    // Lock the driver and set up the transfer
    uartStatus = UART_BUSY;
    txComplete = 0;
    rxComplete = 0;
    rxIndex = 0;

    pTxData = pData;
    txLen = Size;

    // Enable the TXE interrupt to start the transmission
    USART2->CR1 |= USART_CR1_TXEIE;

    return UART_OK;
}

UART_Status InterruptUART_GetStatus(void) {
    return uartStatus;
}

uint8_t* InterruptUART_GetRxBuffer(void) {
    return rxBuffer;
}

// --- INTERRUPT SERVICE ROUTINE ---
void USART2_IRQHandler(void) {
    // Handle RX
    if ((USART2->SR & USART_SR_RXNE) && (USART2->CR1 & USART_CR1_RXNEIE)) {
        uint8_t received_byte = (uint8_t)USART2->DR;
        if (rxIndex < RX_BUFFER_SIZE) {
            rxBuffer[rxIndex++] = received_byte;
        }
        if (rxIndex >= RX_BUFFER_SIZE) {
            rxComplete = 1;
        }
    }

    // Handle TX
    if ((USART2->SR & USART_SR_TXE) && (USART2->CR1 & USART_CR1_TXEIE)) {
        if (txLen > 0) {
            USART2->DR = *pTxData++;
            txLen--;
        }
        if (txLen == 0) {
            // Disable TXE interrupt and mark TX as complete
            USART2->CR1 &= ~USART_CR1_TXEIE;
            txComplete = 1;
        }
    }

    // If both TX and RX are complete, the entire loopback is done
    if (txComplete && rxComplete) {
        uartStatus = UART_OK;
    }
}
