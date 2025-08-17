#include "dma_uart.h"

// --- Private Driver State ---
static volatile uint8_t tx_complete = 0;
static volatile uint8_t rx_complete = 0;

void DMA_UART_Init(void) {
    // 1. Enable Clocks (GPIOA, USART2, DMA1)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    // 2. Configure GPIOs
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos);

    // 3. Configure USART
    USART2->CR1 &= ~USART_CR1_UE;
    USART2->BRR = 0x1117; // 9600 baud
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

    // 4. Tell USART to use DMA for TX and RX
    USART2->CR3 |= (USART_CR3_DMAT | USART_CR3_DMAR);

    // 5. Configure DMA Streams (but don't enable yet)
    // --- TX on DMA1, Stream 6, Channel 4 ---
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while(DMA1_Stream6->CR & DMA_SxCR_EN);
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;
    DMA1_Stream6->CR |= (4 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_TCIE;

    // --- RX on DMA1, Stream 5, Channel 4 ---
    DMA1_Stream5->CR &= ~DMA_SxCR_EN;
    while(DMA1_Stream5->CR & DMA_SxCR_EN);
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;
    DMA1_Stream5->CR |= (4 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_MINC | DMA_SxCR_TCIE;

    // 6. Configure NVIC for DMA interrupts
    NVIC_EnableIRQ(DMA1_Stream6_IRQn); // TX
    NVIC_EnableIRQ(DMA1_Stream5_IRQn); // RX

    // 7. Enable USART
    USART2->CR1 |= USART_CR1_UE;
}

// THIS FUNCTION HAS BEEN UPDATED FOR ROBUSTNESS
void DMA_UART_StartLoopback(uint8_t* pTxData, uint8_t* pRxData) {
    // --- Reset state and clear all flags before starting ---
    tx_complete = 0;
    rx_complete = 0;

    // Disable streams to ensure a clean state
    DMA1_Stream5->CR &= ~DMA_SxCR_EN; // RX
    DMA1_Stream6->CR &= ~DMA_SxCR_EN; // TX
    while((DMA1_Stream5->CR & DMA_SxCR_EN) || (DMA1_Stream6->CR & DMA_SxCR_EN)); // Wait

    // Clear all possible interrupt flags for both streams
    DMA1->HIFCR = (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5);
    DMA1->HIFCR = (DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6);

    // --- Configure and start the transfer ---
    // Configure RX stream
    DMA1_Stream5->M0AR = (uint32_t)pRxData;
    DMA1_Stream5->NDTR = 50;

    // Configure TX stream
    DMA1_Stream6->M0AR = (uint32_t)pTxData;
    DMA1_Stream6->NDTR = 50;

    // Enable the streams to begin the transfer
    DMA1_Stream5->CR |= DMA_SxCR_EN; // RX
    DMA1_Stream6->CR |= DMA_SxCR_EN; // TX
}

uint8_t DMA_UART_IsTransferComplete(void) {
    return (tx_complete && rx_complete);
}

// --- DMA Interrupt Service Routines ---
void DMA1_Stream6_IRQHandler(void) { // TX ISR
    if (DMA1->HISR & DMA_HISR_TCIF6) {
        tx_complete = 1;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF6; // Clear flag
    }
}

void DMA1_Stream5_IRQHandler(void) { // RX ISR
    if (DMA1->HISR & DMA_HISR_TCIF5) {
        rx_complete = 1;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF5; // Clear flag
    }
}
