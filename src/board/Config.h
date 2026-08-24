#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>

// --- Sensör Tipi ---
#define USE_GY87

// --- UART Pin Atamaları ---
#define SBUS_UART_INDEX  0   // 0: Serial1/UART0, 1: Serial2/UART1
#define PIN_SBUS_RX     1   // UART0 RX — SBUS alıcı (transistör ile invert)
#define SBUS_UART_BAUD  100000
#define SBUS_UART_CONFIG SERIAL_8E2
#define PIN_GPS_TX      8
#define PIN_GPS_RX      9
#define GPS_MODULE_ENABLED 0
#define GPS_UART_BAUD 9600

// --- I2C Pin Atamaları ---
#define PIN_SDA         4
#define PIN_SCL         5

// --- User feedback GPIO ---
#define PIN_BUZZER      22
#define PIN_ARM_LED_RED   7
#define PIN_ARM_LED_GREEN 0

// --- Debug Timing GPIO Pinleri ---
#define PIN_DEBUG_CONSUME 2
#define PIN_DEBUG_PID     3
#define PIN_DEBUG_MIXER   6

// --- Bench admin override ---
// GP20 ile GP21 kisa devre edilirse bench/admin force-arm kapisi acilir.
// Normal ucus icin bu jumper takili olmamalidir.
#define BENCH_ADMIN_FORCE_ARM_ENABLED 1
#define PIN_BENCH_ADMIN_GND   20
#define PIN_BENCH_ADMIN_SENSE 21

// --- PIO UART MAVLink telemetry radio ---
#define PIN_TELEM_TX      12
#define PIN_TELEM_RX      13
#ifndef TELEMETRY_UART_ENABLED
#define TELEMETRY_UART_ENABLED 1
#endif
#ifndef TELEMETRY_UART_BAUD
#define TELEMETRY_UART_BAUD 57600
#endif

// --- MAVLink bench / GCS transport ---
#define MAVLINK_USB_ENABLED 1

// --- Blackbox output routing ---
// TELEMETRY: binary blackbox kayitlari MAVLink telemetri UART hattina akar.
// SD:        yalniz SD karta yazar, telemetri linkini zorlamaz.
// BOTH:      once SD karta, sonra telemetri hattina ayni kaydi yollar.
#define BLACKBOX_OUTPUT_TELEMETRY 1
#define BLACKBOX_OUTPUT_SD        2
#define BLACKBOX_OUTPUT_BOTH      3
#ifndef BLACKBOX_OUTPUT_MODE
#define BLACKBOX_OUTPUT_MODE BLACKBOX_OUTPUT_TELEMETRY
#endif

// --- Blackbox SD card over SPI ---
// BLACKBOX_OUTPUT_SD veya BOTH secilecekse harici SD kart modulu takilmali.
// Binary blackbox kayitlari /AEROPICO.BBX dosyasina eklenir.
#ifndef BLACKBOX_SD_ENABLED
#define BLACKBOX_SD_ENABLED 0
#endif
#define PIN_BLACKBOX_SPI_SCK  10
#define PIN_BLACKBOX_SPI_MOSI 11
#define PIN_BLACKBOX_SPI_MISO 14
#define PIN_BLACKBOX_SPI_CS   15
#define BLACKBOX_SPI_HZ       8000000UL
#define BLACKBOX_SD_FILE      "/AEROPICO.BBX"

// --- PWM Servo Çıkışları ---
#define PIN_AILERON     16
#define PIN_ELEVATOR    17
#define PIN_RUDDER      18
#define PIN_THROTTLE    19

// --- Battery / Brownout ADC ---
// Varsayilan acik: divider/pin hatasi preflight ve health tarafinda gorunur olmalidir.
#ifndef BATTERY_ADC_ENABLED
#define BATTERY_ADC_ENABLED 1
#endif
#define PIN_BATTERY_ADC 26
#define BATTERY_ADC_CHANNEL 0
#define BATTERY_VOLTAGE_DIVIDER_RATIO 11.0f
#define BATTERY_CELL_COUNT 3
#define BATTERY_NOMINAL_VOLTAGE 11.1f
#define BATTERY_CAPACITY_MAH 3300
#define BATTERY_C_RATING 40
#define BATTERY_MIN_VOLTAGE 10.5f
#define BATTERY_MAX_VOLTAGE 12.8f
#define BATTERY_BROWNOUT_VOLTAGE 9.6f

// --- Default module setup profile ---
// Runtime parametreler flash'a kaydedilebilir; bu varsayilanlar sadece temiz/force profile
// acilisinda ilk setup snapshot'ini belirler.
#ifndef AEROPICO_DEFAULT_EN_BARO
#define AEROPICO_DEFAULT_EN_BARO 1
#endif
#ifndef AEROPICO_DEFAULT_EN_MAG
#define AEROPICO_DEFAULT_EN_MAG 1
#endif
#ifndef AEROPICO_DEFAULT_EN_GPS
#define AEROPICO_DEFAULT_EN_GPS GPS_MODULE_ENABLED
#endif
#ifndef AEROPICO_DEFAULT_EN_BATT
#define AEROPICO_DEFAULT_EN_BATT 0
#endif
#ifndef AEROPICO_DEFAULT_EN_RC
#define AEROPICO_DEFAULT_EN_RC 1
#endif

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
#define RC_MODE_CHANNEL     4
#define RC_MODE_STABILIZE_THRESHOLD 1500

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

#endif  // BOARD_CONFIG_H
