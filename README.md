![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![C++](https://img.shields.io/badge/Language-C++-blue)
# AeroPico FC: Raspberry Pi Pico Fixed-Wing Flight Controller

**AeroPico FC** is a modular, lightweight flight controller software designed for fixed-wing aircraft, leveraging the power of the **Raspberry Pi Pico**. It provides a customizable, low-cost flight control solution for hobbyists and researchers.

> **Note:** This project is currently in the **prototype phase**. It is intended for educational and experimental purposes and should not be used in critical industrial or commercial applications.

---

### 🏗 System Architecture

The software follows a modular design to separate low-level hardware abstraction from high-level flight logic.

### 📂 Project Structure

| Module | Description |
| --- | --- |
| `src/main.cpp` | Entry point managing the `setup()` and `loop()` control cycles. |
| `src/config.h` | Central configuration for pins, PID constants, and RC parameters. |
| `src/core/` | Flight management, Madgwick sensor fusion, and PID controller loops. |
| `src/drivers/` | Hardware abstraction layers (MPU6050, GY-87, SBUS, PWM). |
| `src/telemetry/` | MAVLink-based communication protocols for GCS integration. |
| `src/utils/` | Logger and mathematical helper functions. |
```mermaid
graph TD
    %% Sensors & Input Layer
    IMU["MPU6050 IMU"] --> SP["Sensor Processing\nAcc & Gyro"]
    SP --> MF["Madgwick Filter\nQuaternion → Euler"]
    RC["RC Receiver\nChannels 1–4"] --> MX["🔒 Mutex\nData Protection"]
    GPS["GPS Module\nGlobal Position"] --> MX
    
    %% Core Processing Layer
    MF --> MX
    MX -- "Euler Angles & RC" --> OP["Outer Loop: Angle PID"]
    MX -. "Gyro Rates" .-> IP["Inner Loop: Rate PID"]
    MX -- "Position & Heading" --> NAV["Navigation & MAVLink"]

    %% Output & Safety Layer
    OP --> IP
    IP --> FM["Mixer\nPID + RC → Servos"]
    FM --> FSM{"Failsafe Monitor"}
    FSM --> SC["Safety Limits\nPWM 1000–2000μs"]

    %% Actuation
    SC --> A["Aileron"]
    SC --> E["Elevator"]
    SC --> R["Rudder"]
    SC --> T["Throttle (ESC)"]

    %% Communication
    NAV <==> MAV["MAVLink Interface\nTelemetry & GCS"]

    %% Styling
    style MX fill:#f96,stroke:#333
    style FSM fill:#f66,stroke:#333
    style MAV fill:#6cf,stroke:#333

```
---

### 🚀 Roadmap

| Feature | Status |
| --- | --- |
| Basic Flight Control Loop | ✅ Completed |
| SBUS RC Input | ✅ Completed |
| MPU6050 + GY-87 Support | ✅ Completed |
| MAVLink Telemetry | ⏳ In Progress |
| Failsafe & Error Tolerance | ⏳ In Progress |
| GCS Integration | 📅 Planned |

---

### 🛠 How to Build

1. **Clone** this repository.
2. Open the project in **PlatformIO** (VS Code).
3. Ensure your `platformio.ini` is configured for the `earlephilhower` core.
4. Run the **Build** command.
5. Copy the generated `firmware.uf2` file to your Raspberry Pi Pico in **BOOTSEL** mode.

---

### 💡 Contribute

I welcome contributions! If you have suggestions, bug reports, or feature requests, feel free to **open an Issue** or submit a **Pull Request**.

*Developed by Muhammed Fatih Emre Özçelik*
*Copyright © 2026 Muhammed Fatih Emre Özçelik. All rights reserved.*

---

