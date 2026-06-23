#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Sensör Tipi ---
#define USE_GY87

// --- Donanım Pin Atamaları ---
#define PIN_SDA 4
#define PIN_SCL 5
#define PIN_SBUS 6

// Çıkış (PWM) Pinleri
#define PIN_AILERON 16
#define PIN_ELEVATOR 17
#define PIN_RUDDER 18
#define PIN_THROTTLE 19

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

#endif // CONFIG_H
