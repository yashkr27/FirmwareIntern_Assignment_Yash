
---

## 📝 Phase 1 – STM32 Foundations
- Completed the **Udemy course** on STM32 bare-metal driver development.
- Key topics: GPIO configuration, RCC/clock trees, USART/UART, DMA, timers, I2C/SPI, NVIC/interrupts.
- Deliverable: `Summary.pdf` – contains notes, concepts learned, and key challenges.

---

## 📝 Phase 2 – UART Driver Development
- Implemented a **bare-metal UART driver** for loopback testing.
- Configurations:
  - Baud: **9600**
  - Data format: **8N1 (8 data bits, no parity, 1 stop bit)**
  - Data buffer: 50-byte test string
- Parts implemented:
  - **Polling-based loopback**
  - **Interrupt-based loopback**
  - **DMA-based loopback** (with circular mode exploration)
- Deliverables:
  - Source files (`polling_uart.*`, `interrupt_uart.*`, `dma_uart.*`)
  - `README.md` explaining:
    - GPIO speed vs signal integrity
    - Baud rate calculation
    - DMA NDTR usage
    - USART error flags and recovery
  - Demo video + debugger screenshot

---

## 📝 Phase 3 – STM32 Custom Board Design
- Designed a **2-layer PCB** with STM32F405 MCU, including:
  - 3.3V LDO regulator
  - 8 MHz crystal
  - Reset button
  - SWD debug header
  - UART header
  - Power and user LEDs
  - USB interface
- Deliverables:
  - `schematic.pdf` and `pcb_layout.pdf`
  - `bringup_notes.md` (assumptions, component reasoning, pinout diagram, bring-up checklist, routing notes)
  - `gerber_files/` for fabrication
  - `3d_render.png` (optional)

---

## ✅ Submission Notes
- Each phase is in its own folder with clear structure.
- Code is well-documented with comments.
- Markdown notes include pinout diagrams and bring-up instructions.
- Gerbers are fabrication-ready and verified in Gerber Viewer.
- Without the git attributes file, loopback.mp4 would not be pushed(needed git lfs, due to its size).
---

 **Author:** <Yash Kr. Singh>  
