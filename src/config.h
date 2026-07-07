#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Sensör Tipi ---
#define USE_GY87

// --- UART Pin Atamaları ---
#define SBUS_UART_INDEX  0   // 0: Serial1/UART0, 1: Serial2/UART1
#define PIN_SBUS_RX     1   // UART0 RX — SBUS alıcı (transistör ile invert)
#define PIN_GPS_TX      8
#define PIN_GPS_RX      9

// --- I2C Pin Atamaları ---
#define PIN_SDA         4
#define PIN_SCL         5

// --- Debug Timing GPIO Pinleri ---
#define PIN_DEBUG_CONSUME 2
#define PIN_DEBUG_PID     3
#define PIN_DEBUG_MIXER   6

// --- PIO UART — ESP32-CAM MAVLink ---
#define PIN_ESP_TX      12
#define PIN_ESP_RX      13

// --- PWM Servo Çıkışları ---
#define PIN_AILERON     16
#define PIN_ELEVATOR    17
#define PIN_RUDDER      18
#define PIN_THROTTLE    19

// --- Uçuş Parametreleri ---
#define PID_P_GAIN 0.5
#define PID_I_GAIN 0.01
#define PID_D_GAIN 0.05

// Sensör filtreleme
#define SENSOR_MEDIAN_WINDOW 3
#define SENSOR_STALE_TIMEOUT_US 20000
#define SENSOR_DEBUG_LOG_ENABLED 0
#define SENSOR_DEBUG_LOG_INTERVAL_MS 250
#define I2C_DMA_TIMEOUT_US 2000

#define PWM_MIN 1000
#define PWM_MAX 2000
#define PWM_NEUTRAL 1500

// RC kanal eşlemeleri
#define RC_ROLL_CHANNEL     0
#define RC_PITCH_CHANNEL    1
#define RC_THROTTLE_CHANNEL 2
#define RC_YAW_CHANNEL      3

// Açı ve Rate PID Parametreleri
#define ANGLE_P_GAIN 2.0
#define ANGLE_I_GAIN 0.05
#define ANGLE_D_GAIN 0.1
#define RATE_P_GAIN 0.1
#define RATE_I_GAIN 0.01
#define RATE_D_GAIN 0.01

#define PID_INTEGRAL_LIMIT 100.0f
#define PID_SERVO_CORRECTION_LIMIT 500.0f

// Limitler
#define MAX_ROLL_ANGLE  30.0f
#define MAX_PITCH_ANGLE 20.0f
#define YAW_SERVO_GAIN  0.7f
#define MAX_YAW_RATE    100.0f

// Watchdog & Failsafe
#define WATCHDOG_TIMEOUT_MS     2000
#define FAILSAFE_TIMEOUT_MS     500
#define MAVLINK_RC_OVERRIDE_TIMEOUT_MS 1000
#define FAILSAFE_THROTTLE       PWM_MIN
#define FAILSAFE_AILERON        PWM_NEUTRAL
#define FAILSAFE_ELEVATOR       PWM_NEUTRAL
#define FAILSAFE_RUDDER         PWM_NEUTRAL

#endif
