/*
   MPU6500.cpp - Library for interfacing with an MPU-6500 MotionTracking device.
*/

#include "Arduino.h"
#include "MPU6500.h"
#include "MPU6500RM.h"
#include "Wire.h"
#include "SPI.h"

uint8_t readRegister(uint8_t addr);
void writeRegister(uint8_t addr, uint8_t val);

MPU6500::MPU6500(): spi(NULL) {}

bool MPU6500::_setupDone = false;
MPU6500::intr MPU6500::_interface = MPU6500::I2C;

uint8_t buf[15];

void MPU6500::setup(MPU6500::intr interface, void *intr_handler)
{
  if (!MPU6500::_setupDone) {

    MPU6500::_interface = interface;

    switch (interface) {
      case MPU6500::I2C : {
          Wire.begin();
          Wire.setClock(I2C_FSCL);
          Wire.beginTransmission(I2C_ADDR);
          Wire.write(PWR_MGMT_1);
          Wire.write(0);
          Wire.endTransmission(true);
          // TODO: writeRegister(PWR_MGMT_1, 0); or 0x80 ? and what about SIGNAL_PATH_RESET or USER_CTRL?
        } break;

      case MPU6500::SPI : {
          pinMode(SPI_CS, OUTPUT);
          digitalWrite(SPI_CS, HIGH);
          this->spi = (SPIClass *)intr_handler;
          this->spi->begin();

          writeRegister(PWR_MGMT_1, 0x81);
          delay(100);
          writeRegister(SIGNAL_PATH_RESET, 0x07);
          delay(100);

          writeRegister(ACCEL_CONFIG,   0b00000000);
          writeRegister(ACCEL_CONFIG_2, 0x08);

        } break;
    }

    MPU6500::_setupDone = true;
  }
}

void MPU6500::i2cSample(double (&values)[6])
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDR, (uint8_t)14);
  
    // Get Accelerometer values
  
    values[0] = (int16_t)(Wire.read() << 8 | Wire.read());
    values[1] = (int16_t)(Wire.read() << 8 | Wire.read());
    values[2] = (int16_t)(Wire.read() << 8 | Wire.read());
  
    // Do not care about temperature
  
    Wire.read(); Wire.read();
  
    // Get Gyroscope values
  
    values[3] = (int16_t)(Wire.read() << 8 | Wire.read());
    values[4] = (int16_t)(Wire.read() << 8 | Wire.read());
    values[5] = (int16_t)(Wire.read() << 8 | Wire.read());
  
  
    Wire.endTransmission(true);
}

void MPU6500::rawspiSample(uint8_t (&values)[12])
{

  buf[0] = 0x80 | ACCEL_XOUT_H;

  this->spi->beginTransaction(SPISettings(SPI_DATA_FREQ, MSBFIRST, SPI_MODE3));
  digitalWrite(SPI_CS, LOW);
  this->spi->transfer(buf, 15);
  digitalWrite(SPI_CS, HIGH);
  this->spi->endTransaction();

  values[0] = buf[1]; values[1] = buf[2];
  values[2] = buf[3]; values[3] = buf[4];
  values[4] = buf[5]; values[5] = buf[6];

  values[6 ] = buf[9 ]; values[7 ] = buf[10];
  values[8 ] = buf[11]; values[9 ] = buf[12];
  values[10] = buf[13]; values[11] = buf[14];
  
}

void MPU6500::spiSample(double (&values)[6])
{

  buf[0] = 0x80 | ACCEL_XOUT_H;

  this->spi->beginTransaction(SPISettings(SPI_DATA_FREQ, MSBFIRST, SPI_MODE3));
  digitalWrite(SPI_CS, LOW);
  this->spi->transfer(buf, 15);
  digitalWrite(SPI_CS, HIGH);
  this->spi->endTransaction();

  values[0] = (int16_t)(buf[1] << 8 | buf[2]);
  values[1] = (int16_t)(buf[3] << 8 | buf[4]);
  values[2] = (int16_t)(buf[5] << 8 | buf[6]);

  values[3] = (int16_t)(buf[9] << 8 | buf[10]);
  values[4] = (int16_t)(buf[11] << 8 | buf[12]);
  values[5] = (int16_t)(buf[13] << 8 | buf[14]);
  
}

void MPU6500::sample(double (&values)[6])
{
  switch (MPU6500::_interface) {
    case MPU6500::I2C :
      i2cSample(values);
      break;
    case MPU6500::SPI :
      spiSample(values);
      break;
  }
}

void MPU6500::calibrate()
{
  // TODO: burst write

  writeRegister(X_OFFS_USR_H, (uint8_t)(0));
  writeRegister(X_OFFS_USR_L, (uint8_t)(0));
  writeRegister(Y_OFFS_USR_H, (uint8_t)(0));
  writeRegister(Y_OFFS_USR_L, (uint8_t)(0));
  writeRegister(Z_OFFS_USR_H, (uint8_t)(0));
  writeRegister(Z_OFFS_USR_L, (uint8_t)(0));


  double meanVals[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double vals[6]     = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  for (int i = 0; i < 10000; i++) {
    if (i >= 100 - 1) {
      this->sample(vals);
      ets_delay_us(SAMPLING_DELAY);
      for (int k = 0; k < 6; k++) {
        meanVals[k] += vals[k];
      }
    }
  }

  for (int k = 0; k < 6; k++) {
    meanVals[k] /= 9900;
    meanVals[k] *= -1;
    meanVals[k] /= 4;
  }

  writeRegister(X_OFFS_USR_H, (uint8_t)((int16_t)meanVals[3] >> 8));
  writeRegister(X_OFFS_USR_L, (uint8_t)((int16_t)meanVals[3]     ));
  writeRegister(Y_OFFS_USR_H, (uint8_t)((int16_t)meanVals[4] >> 8));
  writeRegister(Y_OFFS_USR_L, (uint8_t)((int16_t)meanVals[4]     ));
  writeRegister(Z_OFFS_USR_H, (uint8_t)((int16_t)meanVals[5] >> 8));
  writeRegister(Z_OFFS_USR_L, (uint8_t)((int16_t)meanVals[5]     ));

}

uint8_t MPU6500::spiReadRegister(uint8_t addr) {

  this->spi->beginTransaction(SPISettings(SPI_SETUP_FREQ, MSBFIRST, SPI_MODE3));

  digitalWrite(SPI_CS, LOW);
  uint16_t reg = this->spi->transfer16((0x80 | addr) << 8);
  digitalWrite(SPI_CS, HIGH);

  this->spi->endTransaction();

  return (uint8_t)reg;

}

void MPU6500::spiWriteRegister(uint8_t addr, uint8_t val) {

  this->spi->beginTransaction(SPISettings(SPI_SETUP_FREQ, MSBFIRST, SPI_MODE3));
  digitalWrite(SPI_CS, LOW);
  this->spi->transfer16(((0x00 | addr) << 8) | val);
  digitalWrite(SPI_CS, HIGH);
  this->spi->endTransaction();
  
}

uint8_t MPU6500::i2cReadRegister(uint8_t addr) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(addr);
  Wire.endTransmission(false);
  Wire.requestFrom(I2C_ADDR, (uint8_t)1);
  uint8_t value = Wire.read();
  Wire.endTransmission(true);
  return value;
}

void MPU6500::i2cWriteRegister(uint8_t addr, uint8_t val) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(addr);
  Wire.write(val);
  Wire.endTransmission(true);
}

uint8_t MPU6500::readRegister(uint8_t addr) {
  switch (MPU6500::_interface) {
    case MPU6500::I2C : return i2cReadRegister(addr);
    case MPU6500::SPI : return spiReadRegister(addr);
  }
}

void MPU6500::writeRegister(uint8_t addr, uint8_t val) {
  switch (MPU6500::_interface) {
    case MPU6500::I2C : i2cWriteRegister(addr, val); break;
    case MPU6500::SPI : spiWriteRegister(addr, val); break;
  }
}
