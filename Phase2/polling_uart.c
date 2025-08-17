/**
 * @file polling_uart.c
 * @brief Bare-metal polling UART driver for STM32F407xx.
 */

#include "polling_uart.h"

// A simple timeout value for polling loops to prevent getting stuck.
#define UART_POLL_TIMEOUT 0x0000FFFF

/**
 * @brief Initializes USART2 for polling (blocking) communication.
 * @details Configures USART2 for 9600 baud, 8N1. Enables PA2 (TX) and PA3 (RX).
 * @return UART_Status: UART_OK on success.
 */
UART_Status PollingUART_Init(void) {
    // 1. Enable peripheral clocks in RCC
    //------------------------------------
    // Enable clock for GPIOA port
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    // Enable clock for USART2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // 2. Configure GPIO Pins for Alternate Function
    //-----------------------------------------------
    // Set PA2 (TX) and PA3 (RX) to alternate function mode.
    // MODERy[1:0] = 10 for alternate function
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3); // Clear bits first
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1); // Set to AF mode (10)

    // Set PA3 (RX) to have a pull-up resistor.
    // PUPDRy[1:0] = 01 for pull-up
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR3; // Clear bits first
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_0; // Set to Pull-up (01)

    // Set the alternate function mapping for PA2 and PA3 to AF7 (USART2).
    // Pins 0-7 are configured in AFR[0] (AFRL). Each pin needs 4 bits.
    // AFRLy[3:0] = 0111 for AF7
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3); // Clear bits first
    GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos);

    // 3. Configure USART2 Peripheral
    //--------------------------------
    // It's good practice to disable USART before configuration
    USART2->CR1 &= ~USART_CR1_UE;

    // Set word length to 8 data bits (M bit = 0)
    USART2->CR1 &= ~USART_CR1_M;

    // Set parity to no parity (PCE bit = 0)
    USART2->CR1 &= ~USART_CR1_PCE;

    // Set 1 stop bit (STOP bits = 00)
    USART2->CR2 &= ~USART_CR2_STOP;

    // Set baud rate to 9600.
    // Assuming the APB1 clock (f_CK) is 42 MHz.
    // USARTDIV = 42,000,000 / (16 * 9600) = 273.4375
    // Mantissa = 273 (0x111), Fraction = 0.4375 * 16 = 7 (0x7)
    // BRR value = (Mantissa << 4) | Fraction = 0x1117
    USART2->BRR = 0x1117;

    // Enable Transmitter (TE) and Receiver (RE)
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

    // Finally, enable the USART peripheral
    USART2->CR1 |= USART_CR1_UE;

    return UART_OK;
}

/**
 * @brief Sends a block of data over USART2 using polling.
 * @param pData Pointer to the data buffer to be transmitted.
 * @param Size Number of bytes to transmit.
 * @return UART_Status: UART_OK on success, UART_TIMEOUT if a byte times out.
 */
UART_Status PollingUART_Transmit(uint8_t *pData, uint16_t Size) {
    for (uint16_t i = 0; i < Size; i++) {
        volatile uint32_t timeout = UART_POLL_TIMEOUT;

        // Wait until the Transmit Data Register (TXE) is empty.
        while (!(USART2->SR & USART_SR_TXE)) {
            if (--timeout == 0) {
                return UART_TIMEOUT;
            }
        }

        // Write the data to the Data Register.
        USART2->DR = pData[i];
    }

    // After the last byte is written to DR, wait for Transmission Complete (TC)
    // to ensure all data has physically left the shifter.
    volatile uint32_t timeout = UART_POLL_TIMEOUT;
    while (!(USART2->SR & USART_SR_TC)) {
         if (--timeout == 0) {
            return UART_TIMEOUT;
        }
    }

    return UART_OK;
}

/**
 * @brief Receives a block of data over USART2 using polling.
 * @param pData Pointer to the buffer to store received data.
 * @param Size Number of bytes to receive.
 * @return UART_Status: UART_OK on success, UART_TIMEOUT if a byte times out.
 */
UART_Status PollingUART_Receive(uint8_t *pData, uint16_t Size) {
    for (uint16_t i = 0; i < Size; i++) {
        volatile uint32_t timeout = UART_POLL_TIMEOUT;

        // Wait until the Receive Data Register (RXNE) is not empty.
        while (!(USART2->SR & USART_SR_RXNE)) {
            if (--timeout == 0) {
                return UART_TIMEOUT;
            }
        }

        // Read the data from the Data Register. This also clears the RXNE flag.
        pData[i] = (uint8_t)(USART2->DR & 0xFF);
    }

    return UART_OK;
}
