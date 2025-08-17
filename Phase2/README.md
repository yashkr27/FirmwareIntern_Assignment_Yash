# Phase 2: Bare-Metal UART Driver Development

This directory contains the source code for three bare-metal UART drivers for the STM32F407G microcontroller, demonstrating communication via polling, interrupts, and Direct Memory Access (DMA).

The objective is to transmit a fixed 50-byte array and receive it back in a loopback configuration. The tests were conducted using the Renode simulation environment.

## Technical Implementation Details

### 1. GPIO Speed Impact on Signal Integrity

The speed of a GPIO pin, configured in the `GPIOx_OSPEEDR` register, determines the rise and fall times of the output signal.

* **Low Speed:** Slower rise/fall times. This reduces high-frequency noise (EMI) and prevents signal reflections (ringing) on longer traces. It is suitable for low-frequency signals like button inputs or simple I2C.
* **High Speed:** Faster rise/fall times. This is necessary for high-frequency communication protocols like SPI or fast UART to ensure the signal reaches its valid logic level within the bit period.

**Impact on UART:**
For a baud rate of 9600, the signal frequency is relatively low. However, setting the GPIO speed to "High" is a safe practice for communication peripherals. It ensures sharp signal edges, which improves the reliability of the receiver's sampling logic. A slow rise time could cause the receiver to sample the bit at an incorrect voltage level, leading to bit errors.

---

### 2. Baud Rate Calculation (Oversampling by 16 vs. 8)

The baud rate is determined by the peripheral clock frequency (`f_CK`) and the value of the `USART_BRR` register. The formula depends on the oversampling mode set by the `OVER8` bit in `USART_CR1`.

**A. Oversampling by 16 (OVER8 = 0)**
This is the default, more robust mode, as it takes more samples to determine the bit value.

* **Formula:**
    $$ \text{Baudrate} = \frac{f_{CK}}{16 \times \text{USARTDIV}} $$
* **Calculation for 9600 Baud:**
    Assuming the APB1 clock (`f_CK`) is 42 MHz:
    $$ \text{USARTDIV} = \frac{42,000,000}{16 \times 9600} = 273.4375 $$
    The `USART_BRR` register requires the integer and fractional parts separately:
    * `DIV_Mantissa` (integer part) = **273**
    * `DIV_Fraction` (fractional part) = $0.4375 \times 16 = \textbf{7}$
    * **BRR Value:** `(273 << 4) | 7` = `0x1117`

**B. Oversampling by 8 (OVER8 = 1)**
This mode allows for higher maximum baud rates but is less noise-tolerant.

* **Formula:**
    $$ \text{Baudrate} = \frac{f_{CK}}{8 \times \text{USARTDIV}} $$
* **Calculation for 9600 Baud:**
    $$ \text{USARTDIV} = \frac{42,000,000}{8 \times 9600} = 546.875 $$
    * `DIV_Mantissa` = **546**
    * `DIV_Fraction` = $0.875 \times 8 = \textbf{7}$
    * **BRR Value:** The 3 least significant bits of the mantissa are used for the fraction. `(546 & 0xFFF0) | (7 & 0x0007)` = `0x2227`

---

### 3. DMA NDTR Usage and Circular Mode

**A. NDTR (Number of Data to Transfer Register)**
The `DMA_SxNDTR` register is a down-counter that specifies the number of data items (bytes, half-words, or words) to be transferred in a DMA stream.

* **Usage:** Before starting a DMA transfer, you load this register with the size of the buffer you want to transmit or receive (in our case, 50).
* **Behavior:** The DMA controller decrements the `NDTR` register after each successful data transfer. When it reaches zero, the transfer is considered complete, and a **Transfer Complete Interrupt (TCIF)** flag is set in the DMA status register.

**B. Circular Mode**
Circular mode is enabled by setting the `CIRC` bit in the `DMA_SxCR` register.

* **Behavior:** When the `NDTR` register decrements to zero in circular mode, the DMA controller **automatically reloads it** with the initial value and resets the memory address pointer to the beginning of the buffer.
* **Use Case:** This is extremely useful for continuous data streams where the buffer needs to be constantly refilled or processed. For example, receiving data from an ADC or a communication peripheral that sends data indefinitely. The DMA handles the buffer management automatically, allowing the CPU to process a full buffer while the DMA fills the next one in the background. For our single 50-byte transfer, normal mode was sufficient.

---

### 4. USART Error Flag Definitions

The USART peripheral has several error flags in the `USART_SR` register to detect communication problems.

* **ORE (Overrun Error):** This flag is set when a new byte is received in the hardware shift register before the previous byte was read from the data register (`USART_DR`). This means the CPU didn't service the `RXNE` flag fast enough, and data was lost.
* **FE (Framing Error):** This flag is set if the receiver does not detect a valid STOP bit at the end of a data frame. This usually indicates a baud rate mismatch between the transmitter and receiver or a noise-corrupted line.
* **NE (Noise Error):** This flag is set when the receiver detects noise during the data frame. It is typically set along with the `FE` flag and indicates that the signal integrity is poor.

---

### 5. Peripheral Reset and Flag-Clearing Strategies

**A. Peripheral Reset**
The most reliable way to reset a peripheral to its default state is by using the **Reset and Clock Control (RCC)** peripheral.

* **Procedure:**
    1.  Set the corresponding bit in the `RCC_APB1RSTR` (or `APB2RSTR`, `AHB1RSTR`, etc.) register. For USART2, this is `RCC_APB1RSTR_USART2RST`.
    2.  This holds the peripheral in a reset state.
    3.  Clear the bit to release the peripheral from reset.
* **Example:**
    ```c
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;
    ```

**B. Flag-Clearing Strategies**
Different flags in STM32 microcontrollers have different clearing mechanisms, which are specified in the reference manual.

* **Read-Then-Write or Read-Only:** Many flags, like the `RXNE` (Receive Not Empty) flag in USART, are cleared by a specific sequence of operations. For `RXNE`, the flag is cleared automatically when the `USART_DR` is read.
* **Write-1-to-Clear:** Many interrupt flags, like the DMA Transfer Complete flag (`TCIF`), are cleared by writing a '1' to a corresponding bit in a separate flag-clearing register (e.g., `DMA_HIFCR`). Writing a '0' has no effect. This prevents accidental clearing of flags.
* **Never** use a read-modify-write operation on a flag register unless the manual explicitly states it is safe. This can lead to race conditions where you accidentally clear a flag that was set between your read and write operations.

---

### 6. Debugging Techniques Used

The loopback test was developed and verified using the following techniques:

1.  **Renode Simulation:** The entire test was performed in the Renode environment, which eliminated the need for physical hardware. This allowed for rapid iteration and testing of the C code.
2.  **TCP Socket for UART:** The Renode script was configured to expose the virtual USART2 peripheral as a TCP server. This allowed an external Python script to connect to it.
3.  **Python Echo Client:** A Python script (`connect.py`) was written to act as the other end of the communication line. It connected to the TCP server, received the 50 bytes sent by the STM32, and echoed them back. This provided a reliable and repeatable method to test the loopback functionality.
4.  **Renode UART Analyzer:** The `showAnalyzer` command was used to open a window that visually displays the data being transmitted and received by the virtual UART, providing immediate feedback on the test's progress.
5.  **GDB Debugging (for internal state):** While the TCP method verifies the external behavior, `arm-none-eabi-gdb` was used to connect to the running simulation to inspect internal program state, such as the contents of the `rx_data` buffer and the status of control variables, to confirm the C code's internal logic was correct.



