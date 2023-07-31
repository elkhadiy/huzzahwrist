/*
 * MPU6500RM.h - Register mapping for the MPU-6500 Motion Processing Unit.
 */

#ifndef MPU6500RM_H
#define MPU6500RM_H

#include "Arduino.h"

// TODO: Register flags for easier configuration

// Power Management

constexpr uint8_t PWR_MGMT_1        = 107;
constexpr uint8_t SIGNAL_PATH_RESET = 104;

// Configs

constexpr uint8_t ACCEL_CONFIG      = 28;
constexpr uint8_t ACCEL_CONFIG_2    = 29;

// Accelerometer Measurements

constexpr uint8_t ACCEL_XOUT_H = 59;
constexpr uint8_t ACCEL_XOUT_L = 60;
constexpr uint8_t ACCEL_YOUT_H = 61;
constexpr uint8_t ACCEL_YOUT_L = 62;
constexpr uint8_t ACCEL_ZOUT_H = 63;
constexpr uint8_t ACCEL_ZOUT_L = 64;

// Gyroscope Measurements

constexpr uint8_t GYRO_XOUT_H = 67;
constexpr uint8_t GYRO_XOUT_L = 68;
constexpr uint8_t GYRO_YOUT_H = 69;
constexpr uint8_t GYRO_YOUT_L = 70;
constexpr uint8_t GYRO_ZOUT_H = 71;
constexpr uint8_t GYRO_ZOUT_L = 72;

// Gyroscope Offset Registers

constexpr uint8_t X_OFFS_USR_H = 19;
constexpr uint8_t X_OFFS_USR_L = 20;
constexpr uint8_t Y_OFFS_USR_H = 21;
constexpr uint8_t Y_OFFS_USR_L = 22;
constexpr uint8_t Z_OFFS_USR_H = 23;
constexpr uint8_t Z_OFFS_USR_L = 24;

#endif
