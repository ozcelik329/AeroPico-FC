#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Sensör Tipi ---
#define USE_GY87

// --- UART Pin Atamaları ---
#define PIN_SBUS_RX     1   // UART0 RX — SBUS alıcı (transistör ile invert)
#define PIN_GPS_TX      8
#define PIN_GPS_RX      9

// --- I2C Pin Atamaları ---
#define PIN_SDA         4
#define PIN_SCL         5

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

// Limitler
#define MAX_ROLL_ANGLE  30.0f
#define MAX_PITCH_ANGLE 20.0f
#define YAW_SERVO_GAIN  0.7f
#define MAX_YAW_RATE    100.0f

#define LOOP_TIME_MS    20

// Watchdog & Failsafe
#define WATCHDOG_TIMEOUT_MS     2000
#define FAILSAFE_TIMEOUT_MS     500
#define FAILSAFE_THROTTLE       PWM_MIN
#define FAILSAFE_AILERON        PWM_NEUTRAL
#define FAILSAFE_ELEVATOR       PWM_NEUTRAL
#define FAILSAFE_RUDDER         PWM_NEUTRAL

// --- Simülasyon Modu ---
// SITL_MODE aktifken gerçek donanım kullanılmaz
// Sensör verileri simüle edilir, servo çıkışları Serial'a yazdırılır
// Aktif etmek için platformio.ini'ye -DSITL_MODE ekle
#ifdef SITL_MODE
    #define SITL_ROLL_RATE      15.0f   // °/s simüle roll hızı
    #define SITL_PITCH_RATE     5.0f    // °/s simüle pitch hızı
    #define SITL_YAW_RATE       2.0f    // °/s simüle yaw hızı
    #define SITL_ACCEL_NOISE    0.02f   // simüle ivmeölçer gürültüsü
    #define SITL_GYRO_NOISE     0.005f  // simüle jiroskop gürültüsü
#endif
#endif