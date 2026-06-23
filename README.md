![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)
![RP2040](https://img.shields.io/badge/Platform-RP2040-7546C6)
![Experimental](https://shields.io/badge/Status-Experimental-green)
## 🧪 Experimental Branch: RTOS & Multitasking Architecture
This branch is dedicated to transitioning the flight controller from a single-loop polling architecture to a **Preemptive Multitasking Model**. We are refactoring the codebase into isolated, high-reliability tasks.
The purpose of this branch is to experimentally add the following features without damaging the main branch:

| Feature | Status |
| --- | --- |
| FreeRTOS integration | ✅ |
| PWM output via PIO (resetting servo jitter)| largely completed(✅) |
| Sensor reading via DMA, watchdog timer | ⏳ |
| failsafe & heartbeat mechanism | 📅 |
| MAVLink parser (works even without zero, waits for serial port), and blackbox logger (via UART) | 📅 |



### Core Task Structure
We are moving away from the "Main Loop" dependency by distributing critical flight operations into independent FreeRTOS tasks:

* **`Task_Control` (High Priority):** Handles the PID flight loop and sensor fusion. This task is strictly synchronized to prevent jitter.
* **`Task_Sensors` (DMA-driven):** Manages data acquisition from the IMU using DMA, eliminating CPU-bound I/O waits.
* **`Task_Comms` (Background):** Manages asynchronous MAVLink parsing and blackbox UART logging.
* **`Task_Failsafe` (System Guard):** A dedicated heartbeat monitor that triggers the watchdog or RTL protocol if any critical task fails to report within the specified interval.

Unlike the `main` branch, which relies on a procedural execution sequence, this branch experiments with **Concurrency**. By isolating these tasks, we ensure that:
1. Telemetry logging latency cannot affect PID loop timing.
2. Sensor read errors are contained within the `Task_Sensors` boundary.
3. The system remains reactive even under heavy computational load.
   
*Developed by Muhammed Fatih Emre Özçelik* *Copyright © 2026 Muhammed Fatih Emre Özçelik. All rights reserved.*
