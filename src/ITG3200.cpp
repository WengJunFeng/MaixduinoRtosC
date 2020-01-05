#include "ITG3200.h"
#include <cmath>
#include <devices.h>
#include <fpioa.h>

ITG3200::ITG3200(handle_t i2c) {
  this->i2c = i2c;
}

/**
 * @brief Initialization for ITG3200.
 */
void ITG3200::begin() {
  this->device = i2c_get_device(this->i2c, GYRO_ADDRESS, 7);
  i2c_dev_set_clock_rate(this->device, 400000);

  /* send a reset to the device */
  this->resetDevice();

  /* sample rate divider */
  this->setSampleRateDivider(0U);

  /* +/-2000 degrees/s (default value) */
  this->setFullScaleSelection(RANGE_2000_DEG_PER_SEC);
}

/**
 * @brief Read a byte with the register address of ITG3200.
 * @param[in] reg The register address of ITG3200 to read
 * @return The byte that is read from the register
 */
int8_t ITG3200::read(uint8_t reg) {
  int8_t data;
  io_write(this->device, &reg, 1);
  io_read(this->device, (uint8_t *)&data, 1);
  return data;
}

/**
 * @brief Write a byte to the register of the MMA7660
 * @param[in] reg The register address of ITG3200 to write
 * @param[in] data The data to write
 */
void ITG3200::write(uint8_t reg, uint8_t data) {
  this->writeBuffer[0] = reg;
  this->writeBuffer[1] = data;
  io_write(this->device, this->writeBuffer, 2);
}

/**
 * @brief Read a word with the register address of ITG3200.
 * @param[in] reg The register address of ITG3200 to read
 * @return The byte that is read from the register
 */
int16_t ITG3200::read16(uint8_t reg) {
  io_write(this->device, &reg, 1);
  io_read(this->device, readBuffer, 2);
  return (int16_t)((readBuffer[0] << 8) | (readBuffer[1]));
}

void ITG3200::readData(void) {
  writeBuffer[0] = ITG3200_GX_H;
  io_write(this->device, writeBuffer, 1);
  io_read(this->device, readBuffer, 6);
}

/**
 * @brief Get the temperature from ITG3200 that with a on-chip temperature
 * sensor.
 * @return Temperature
 */
double ITG3200::getTemperature() {
  int temp;
  double temperature;
  temp = read16(ITG3200_TMP_H);
  temperature = 35 + ((double)(temp + 13200)) / 280;
  return (temperature);
}

/**
 * @brief Get the contents of the registers in the ITG3200 so as to calculate
 * the angular velocity.
 * @param[out] x
 * @param[out] y
 * @param[out] z
 */
void ITG3200::getXYZ(int16_t *x, int16_t *y, int16_t *z) {
  /* Read 6 bytes in readBuffer */
  readData();
  *x = ((int16_t)((readBuffer[0] << 8) | (readBuffer[1]))) + xOffset;
  *y = ((int16_t)((readBuffer[2] << 8) | (readBuffer[3]))) + yOffset;
  *z = ((int16_t)((readBuffer[4] << 8) | (readBuffer[5]))) + zOffset;
}

/**
 * @brief Get the angular velocity and its unit is degree per second.
 * @param[out] ax
 * @param[out] ay
 * @param[out] az
 */
void ITG3200::getAngularVelocity(float *ax, float *ay, float *az) {
  int16_t x, y, z;
  getXYZ(&x, &y, &z);
  *ax = x / 14.375;
  *ay = y / 14.375;
  *az = z / 14.375;
}

void ITG3200::zeroCalibrate(unsigned int samples, unsigned int sampleDelayMS) {
  int16_t xOffsetTemp = 0;
  int16_t yOffsetTemp = 0;
  int16_t zOffsetTemp = 0;
  int16_t x, y, z;
  xOffset = 0;
  yOffset = 0;
  zOffset = 0;
  getXYZ(&x, &y, &z); //
  for (int i = 0; i < samples; i++) {
    // delay(sampleDelayMS);
    getXYZ(&x, &y, &z);
    xOffsetTemp += x;
    yOffsetTemp += y;
    zOffsetTemp += z;
  }

  xOffset = abs(xOffsetTemp) / samples;
  yOffset = abs(yOffsetTemp) / samples;
  zOffset = abs(zOffsetTemp) / samples;
  if (xOffsetTemp > 0) {
    xOffset = -xOffset;
  }
  if (yOffsetTemp > 0) {
    yOffset = -yOffset;
  }
  if (zOffsetTemp > 0) {
    zOffset = -zOffset;
  }
}

void ITG3200::setXoffset(const int16_t xOffset) {
  this->xOffset = xOffset;
}

void ITG3200::setYoffset(const int16_t yOffset) {
  this->yOffset = yOffset;
}

void ITG3200::setZoffset(const int16_t zOffset) {
  this->zOffset = zOffset;
}

int16_t ITG3200::getXoffset() {
  return this->xOffset;
}

int16_t ITG3200::getYoffset() {
  return this->yOffset;
}

int16_t ITG3200::getZoffset() {
  return this->zOffset;
}

uint8_t ITG3200::whoAmI(void) {
  return this->read(ITG3200_WHO);
}

uint8_t ITG3200::getSampleRateDivider(void) {
  return this->read(ITG3200_SMPL);
}

void ITG3200::setSampleRateDivider(const uint8_t sampleRateDivider) {
  this->write(ITG3200_SMPL, sampleRateDivider);
}

FullScaleRange ITG3200::getFullScaleSelection(void) {
  uint8_t fullScale;
  fullScale = this->getRegisterValue(ITG3200_DLPF, FS_SEL_MASK, FS_SEL_BIT);
  return static_cast<FullScaleRange>(fullScale);
}

void ITG3200::setFullScaleSelection(FullScaleRange fullScale) {
  uint8_t uFullScale = static_cast<uint8_t>(fullScale);
  this->setRegisterBitsValue(ITG3200_DLPF, FS_SEL_MASK, FS_SEL_BIT, uFullScale);
}

void ITG3200::setDigitalLowPassFilter(LowPassFilter lowPassFilterBandwidth) {
  uint8_t ulowPassFilterBandwidth = static_cast<uint8_t>(lowPassFilterBandwidth);
  this->setRegisterBitsValue(ITG3200_DLPF, DLPF_CFG_MASK, DLPF_CFG_BIT, ulowPassFilterBandwidth);
}

LowPassFilter ITG3200::getDigitalLowPassFilter(void) {
  uint8_t lowPassFilterBandwidth;
  lowPassFilterBandwidth = this->getRegisterValue(ITG3200_DLPF, DLPF_CFG_MASK, DLPF_CFG_BIT);
  return static_cast<LowPassFilter>(lowPassFilterBandwidth);
}

bool ITG3200::isRawDataReadyEnabled(void) {
  return this->getRegisterValue(ITG3200_INT_C, RAW_RDY_EN_MASK, RAW_RDY_EN_BIT) == 0b1U;
}

void ITG3200::setRawDataReadyEnabled(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_INT_C, RAW_RDY_EN_MASK, RAW_RDY_EN_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, RAW_RDY_EN_MASK, RAW_RDY_EN_BIT);
  }
}

bool ITG3200::isInterruptEnabled(void) {
  return this->getRegisterValue(ITG3200_INT_C, ITG_RDY_EN_MASK, ITG_RDY_EN_BIT) == 0b1U;
}

void ITG3200::setInterruptEnabled(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_INT_C, ITG_RDY_EN_MASK, ITG_RDY_EN_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, ITG_RDY_EN_MASK, ITG_RDY_EN_BIT);
  }
}

LogicLevelIntOutputPin ITG3200::getLogicLevelIntOutputPin(void) {
  uint8_t logicLevelIntOutputPin;
  logicLevelIntOutputPin = this->getRegisterValue(ITG3200_INT_C, ACTL_MASK, ACTL_BIT);
  if (logicLevelIntOutputPin == 0u) {
    return LOGIC_LEVEL_ACTIVE_HIGH;
  } else {
    return LOGIC_LEVEL_ACTIVE_LOW;
  }
}

void ITG3200::setLogicLevelIntOutputPin(LogicLevelIntOutputPin logicLevelIntOutputPin) {
  if (logicLevelIntOutputPin == LOGIC_LEVEL_ACTIVE_LOW) {
    this->setRegisterBits(ITG3200_INT_C, ACTL_MASK, ACTL_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, ACTL_MASK, ACTL_BIT);
  }
}

DriveTypeIntOutputPin ITG3200::getDriveTypeIntOutputPin(void) {
  uint8_t driveTypeIntOutputPin;
  driveTypeIntOutputPin = this->getRegisterValue(ITG3200_INT_C, OPEN_MASK, OPEN_BIT);
  if (driveTypeIntOutputPin == 0u) {
    return DRIVE_TYPE_PUSH_PULL;
  } else {
    return DRIVE_TYPE_OPEN_DRAIN;
  }
}

void ITG3200::setDriveTypeIntOutputPin(DriveTypeIntOutputPin driveTypeIntOutputPin) {
  if (driveTypeIntOutputPin == DRIVE_TYPE_OPEN_DRAIN) {
    this->setRegisterBits(ITG3200_INT_C, OPEN_MASK, OPEN_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, OPEN_MASK, OPEN_BIT);
  }
}

LatchMode ITG3200::getLatchMode(void) {
  uint8_t latchMode;
  latchMode = this->getRegisterValue(ITG3200_INT_C, LATCH_INT_EN_MASK, LATCH_INT_EN_BIT);
  if (latchMode == 0u) {
    return LATCH_MODE_50US_PULSE;
  } else {
    return LATCH_MODE_LATCH_INT_CLEARED;
  }
}

void ITG3200::setLatchMode(LatchMode latchMode) {
  if (latchMode == LATCH_MODE_LATCH_INT_CLEARED) {
    this->setRegisterBits(ITG3200_INT_C, LATCH_INT_EN_MASK, LATCH_INT_EN_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, LATCH_INT_EN_MASK, LATCH_INT_EN_BIT);
  }
}

LatchClearMethod ITG3200::getLatchClearMethod(void) {
  uint8_t latchClearMethod;
  latchClearMethod = this->getRegisterValue(ITG3200_INT_C, INT_ANYRD_2CLEAR_MASK, INT_ANYRD_2CLEAR_BIT);
  if (latchClearMethod == 0u) {
    return LATCH_2CLEAN_STATUS_REGISTER_READ_ONLY;
  } else {
    return LATCH_2CLEAN_ANY_REGISTER_READ;
  }
}

void ITG3200::setLatchClearMethod(LatchClearMethod latchClearMethod) {
  if (latchClearMethod == LATCH_2CLEAN_ANY_REGISTER_READ) {
    this->setRegisterBits(ITG3200_INT_C, INT_ANYRD_2CLEAR_MASK, INT_ANYRD_2CLEAR_BIT);
  } else {
    this->clearRegisterBits(ITG3200_INT_C, INT_ANYRD_2CLEAR_MASK, INT_ANYRD_2CLEAR_BIT);
  }
}

bool ITG3200::isPllReady(void) {
  return this->getRegisterValue(ITG3200_INT_S, ITG_RDY_MASK, ITG_RDY_BIT) == 0b1U;
}

bool ITG3200::isRawDataReady(void) {
  return this->getRegisterValue(ITG3200_INT_S, RAW_DATA_RDY_MASK, RAW_DATA_RDY_BIT) == 0b1U;
}

void ITG3200::resetDevice(void) {
  this->setRegisterBits(ITG3200_PWR_M, H_RESET_MASK, H_RESET_BIT);
}

bool ITG3200::isSleepMode(void) {
  return this->getRegisterValue(ITG3200_PWR_M, SLEEP_MASK, SLEEP_BIT) == 0b1U;
}

void ITG3200::setSleepMode(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_PWR_M, SLEEP_MASK, SLEEP_BIT);
  } else {
    this->clearRegisterBits(ITG3200_PWR_M, SLEEP_MASK, SLEEP_BIT);
  }
}

bool ITG3200::isStandbyModeX(void) {
  return this->getRegisterValue(ITG3200_PWR_M, STBY_XG_MASK, STBY_XG_BIT) == 0b1U;
}

void ITG3200::setStandbyModeX(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_PWR_M, STBY_XG_MASK, STBY_XG_BIT);
  } else {
    this->clearRegisterBits(ITG3200_PWR_M, STBY_XG_MASK, STBY_XG_BIT);
  }
}

bool ITG3200::isStandbyModeY(void) {
  return this->getRegisterValue(ITG3200_PWR_M, STBY_YG_MASK, STBY_YG_BIT) == 0b1U;
}

void ITG3200::setStandbyModeY(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_PWR_M, STBY_YG_MASK, STBY_YG_BIT);
  } else {
    this->clearRegisterBits(ITG3200_PWR_M, STBY_YG_MASK, STBY_YG_BIT);
  }
}

bool ITG3200::isStandbyModeZ(void) {
  return this->getRegisterValue(ITG3200_PWR_M, STBY_ZG_MASK, STBY_ZG_BIT) == 0b1U;
}

void ITG3200::setStandbyModeZ(bool enable) {
  if (enable == true) {
    this->setRegisterBits(ITG3200_PWR_M, STBY_ZG_MASK, STBY_ZG_BIT);
  } else {
    this->clearRegisterBits(ITG3200_PWR_M, STBY_ZG_MASK, STBY_ZG_BIT);
  }
}

ClockSource ITG3200::getClockSource(void) {
  uint8_t clockSource;
  clockSource = this->getRegisterValue(ITG3200_PWR_M, CLK_SEL_MASK, CLK_SEL_BIT);
  return static_cast<ClockSource>(clockSource);
}

void ITG3200::setClockSource(ClockSource clockSource) {
  uint8_t uClockSource = static_cast<uint8_t>(clockSource);
  this->setRegisterBitsValue(ITG3200_PWR_M, CLK_SEL_MASK, CLK_SEL_BIT, uClockSource);
}

uint8_t ITG3200::getRegisterValue(uint8_t reg, uint8_t mask, uint8_t bit) {
  return (this->read(reg) & mask) >> bit;
}

void ITG3200::setRegisterBitsValue(uint8_t reg, uint8_t mask, uint8_t bit, uint8_t value) {
  uint8_t currentValue;
  currentValue = this->read(reg);
  currentValue &= ~(mask << bit);
  currentValue |= ((mask & value) << bit);
  this->write(reg, currentValue);
}

void ITG3200::setRegisterBits(uint8_t reg, uint8_t mask, uint8_t bit) {
  uint8_t currentValue;
  currentValue = this->read(reg);
  currentValue |= (mask << bit);
  this->write(reg, currentValue);
}

void ITG3200::clearRegisterBits(uint8_t reg, uint8_t mask, uint8_t bit) {
  uint8_t currentValue;
  currentValue = this->read(reg);
  currentValue &= ~(mask << bit);
  this->write(reg, currentValue);
}
