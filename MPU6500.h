/*
   MPU6500.h - Library for interfacing with an MPU-6500 MotionTracking device.
*/

#ifndef MPU6500_H
#define MPU6500_H

#include "Arduino.h"
#include "SPI.h"

constexpr uint32_t SAMPLING_DELAY = 110;
constexpr uint8_t  SPI_CS         = 21;
constexpr uint64_t SPI_SETUP_FREQ = 1000000UL;
constexpr uint64_t SPI_DATA_FREQ  = 20000000UL;
constexpr uint8_t  I2C_ADDR       = 0b1101000; // PS-MPU-6500A-01 page 12
constexpr uint64_t I2C_FSCL       = 400000UL;  // PS-MPU-6500A-01 page 15
constexpr double   ACCEL_SF       = 16384.0; //2048.0; //16384.0;   // PS-MPU-6500A-01 page 10
constexpr double   GYRO_SF        = 131.0;     // TODO: set this up programmatically by reading relevant registers

class MPU6500
{
  public:

    // TODO: a better way of handling multiple implemenetation backends
    // maybe a subclass with (burst)write/readRegister methods
    // see also tradeoff in terms of runtime performance
    enum intr {
      I2C,
      SPI
    };

    MPU6500(SPIClass &vspi);

    // TODO: some error handling and be careful:
    // https://stackoverflow.com/questions/52221727/arduino-wire-library-returning-error-code-7-which-is-not-defined-in-the-library

    void setup(MPU6500::intr interface);

    void calibrate();

    void spiWriteRegister(uint8_t addr, uint8_t val);
    uint8_t spiReadRegister(uint8_t addr);
    uint8_t i2cReadRegister(uint8_t addr);
    void i2cWriteRegister(uint8_t addr, uint8_t val);
    uint8_t readRegister(uint8_t addr);
    void writeRegister(uint8_t addr, uint8_t val);
    void i2cSample(double (&values)[6]);
    void spiSample(double (&values)[6]);
    void rawspiSample(uint8_t (&values)[12]);

    void sample(double (&values)[6]);

    static bool _setupDone;
    static MPU6500::intr _interface;

  private:
    SPIClass &vspi;
};

#endif
