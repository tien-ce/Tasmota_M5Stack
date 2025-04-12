/*
  xsns_118_ac_measure.ino - Unit AC Measure of M5 support for Tasmota

  Copyright (C) 2025 Quang Khanh DO

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>

*/

/*
  This library is built based on https://github.com/RobTillaart/Aht20.git
  All functions of this library are used for educational purposes.
*/

#ifdef USE_I2C
#ifdef USE_ACMEASURE

#define XSNS_118 118
#define XI2C_95 95

#define UNIT_ACMEASURE_ADDR 0x42
#define UNIT_ACMEASURE_VOLTAGE_STRING_REG 0x00
#define UNIT_ACMEASURE_CURRENT_STRING_REG 0x10
#define UNIT_ACMEASURE_POWER_STRING_REG 0x20
#define UNIT_ACMEASURE_APPARENT_POWER_STRING_REG 0x30
#define UNIT_ACMEASURE_POWER_FACTOR_STRING_REG 0x40
#define UNIT_ACMEASURE_KWH_STRING_REG 0x50
#define UNIT_ACMEASURE_VOLTAGE_REG 0x60
#define UNIT_ACMEASURE_CURRENT_REG 0x70
#define UNIT_ACMEASURE_POWER_REG 0x80
#define UNIT_ACMEASURE_APPARENT_POWER_REG 0x90
#define UNIT_ACMEASURE_POWER_FACTOR_REG 0xA0
#define UNIT_ACMEASURE_KWH_REG 0xB0
#define UNIT_ACMEASURE_VOLTAGE_FACTOR_REG 0xC0
#define UNIT_ACMEASURE_CURRENT_FACTOR_REG 0xD0
#define UNIT_ACMEASURE_SAVE_FACTOR_REG 0xE0
#define UNIT_ACMEASURE_GET_READY_REG 0xFC
#define JUMP_TO_BOOTLOADER_REG 0xFD
#define FIRMWARE_VERSION_REG 0xFE
#define I2C_ADDRESS_REG 0xFF

struct UNIT_ACMEASURE
{
  char name[11] = "AC_MEASURE";
  uint8_t count = 0;
  //uint8_t _speed;

  float voltage = NAN;
  float current = NAN;
  //float active_power;
  //float apparent_power;
  //float power_factor;
  //float kWh;

  uint8_t valid = 0;
  uint32_t _lastRead = 0;

} Unit_ACMeasure;

void writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  for(int i = 0; i < length; i++)
  {
    Wire.write(*(buffer + i));
  }
  Wire.endTransmission();
}

void readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
  uint8_t index = 0;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr,length);
  for(int i = 0; i < length; i++)
  {
    buffer[index++] = Wire.read();
  }
}


/* bool begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
  _wire = wire;
  _addr = addr;
  _sda = sda;
  _scl = scl;
  _speed = speed;
  _wire->begin(_sda, _scl, _speed);
  delay(10);
  _wire->beginTransmission(_addr);
  uint8_t error = _wire->endTransmission();
  if (error == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
} */

uint16_t getVoltage(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_VOLTAGE_REG, data, 2);
  uint16_t value = data[0] | (data[1] << 8);
  return value;
}

uint16_t getCurrent(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_CURRENT_REG, data, 2);
  uint16_t value = data[0] | (data[1] << 8);
  return value;
}

uint32_t getPower(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_POWER_REG, data, 4);
  uint32_t value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  return value;
}

uint32_t getApparentPower(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_APPARENT_POWER_REG, data ,4);
  uint32_t value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  return value;
}

uint8_t getPowerFactor(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_POWER_FACTOR_REG, data, 1);
  return data[0];
}

uint32_t getKWH(void)
{
  uint8_t data[4];
  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_KWH_REG, data, 4);
  uint32_t value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  return value;
}

void setKWH(uint32_t value)
{
  writeBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_KWH_REG, (uint8_t *)&value, 4);
}

void getVoltageString(char *str)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR,UNIT_ACMEASURE_VOLTAGE_STRING_REG, (uint8_t *)read_buf, 7);
  memcpy(str,read_buf, sizeof(read_buf));
}

void getCurrentString(char *str)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR,UNIT_ACMEASURE_CURRENT_STRING_REG, (uint8_t *)read_buf, 7);
  memcpy(str, read_buf, sizeof(read_buf));
}

void getPowerString(char *str)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_POWER_STRING_REG, (uint8_t*)read_buf, 7);
  memcpy(str, read_buf, sizeof(read_buf));
}

void getApparentPowerString(char *str)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_APPARENT_POWER_STRING_REG, (uint8_t *)read_buf, 7);
  memcpy(str, read_buf, sizeof(read_buf));
}

void getPowerFactorString(char *str)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_POWER_FACTOR_STRING_REG, (uint8_t *)read_buf, 4);
  memcpy(str, read_buf, sizeof(read_buf));
}

void getKWH(char *str)
{
  char read_buf[11] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_KWH_STRING_REG, (uint8_t *)read_buf, 11);
  memcpy(str, read_buf, sizeof(read_buf));
}

uint8_t getVoltageFactor(void)
{
  uint8_t data[4] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_VOLTAGE_FACTOR_REG, data, 1);
  return data[0];
}

uint8_t getCurrentFactor(void)
{
  uint8_t data[4] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_CURRENT_FACTOR_REG, data, 1);
  return data[0];
}

uint8_t getReady(void)
{
  char read_buf[7] = {0};

  readBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_GET_READY_REG, (uint8_t *)read_buf, 1);
  return read_buf[0];
}

void setVoltageFactor(uint8_t value)
{
  writeBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_VOLTAGE_FACTOR_REG, (uint8_t *)&value, 1);
}

void setCurrentFactor(uint8_t value)
{
  writeBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_CURRENT_FACTOR_REG, (uint8_t *)&value, 1);
}

void saveVoltageCurrentFactor(void)
{
  uint8_t value = 1;

  writeBytes(UNIT_ACMEASURE_ADDR, UNIT_ACMEASURE_SAVE_FACTOR_REG, (uint8_t *)&value, 1);
}

void jumpBootloader(void)
{
  uint8_t value = 1;

  writeBytes(UNIT_ACMEASURE_ADDR, JUMP_TO_BOOTLOADER_REG, (uint8_t *)&value, 1);
}

/* uint8_t setI2CAddress(uint8_t addr)
{
  _wire->beginTransmission(_addr);
  _wire->write(I2C_ADDRESS_REG);
  _wire->write(addr);
  _wire->endTransmission();
  _addr = addr;
  return _addr;
} */

/* uint8_t getI2CAddress(void)
{
  _wire->beginTransmission(_addr);
  _wire->write(I2C_ADDRESS_REG);
  _wire->endTransmission();

  uint8_t RegValue;

  _wire->requestFrom(_addr, 1);
  RegValue = Wire.read();
  return RegValue;
}
 */
uint8_t getFirmwareVersion(void)
{
  Wire.beginTransmission(UNIT_ACMEASURE_ADDR);
  Wire.write(FIRMWARE_VERSION_REG);
  Wire.endTransmission();

  uint8_t RegValue;

  Wire.requestFrom(UNIT_ACMEASURE_ADDR, 1);
  RegValue = Wire.read();
  return RegValue;
}


bool ACMeasureReadData()
{
  if(Unit_ACMeasure.valid)
  {
    Unit_ACMeasure.valid--;
  }

  if(millis() - Unit_ACMeasure._lastRead < 1000) return false;

  Unit_ACMeasure.valid = SENSOR_MAX_MISS;

  uint16_t voltage_read = getVoltage();
  uint16_t current_read = getCurrent();
  Unit_ACMeasure.voltage = (static_cast<float>(voltage_read))/100.0;
  Unit_ACMeasure.current = (static_cast<float>(current_read))/100.0;


  char text_response[50];
  snprintf_P(text_response, sizeof(text_response), PSTR("Voltage : %.2f, Current: %.2f"), Unit_ACMeasure.voltage, Unit_ACMeasure.current);

  AddLog(LOG_LEVEL_INFO,text_response);

  if(isnan(Unit_ACMeasure.voltage) || isnan(Unit_ACMeasure.current)) return false;


  char logVoltage[7];
  getVoltageString(logVoltage);

  AddLog(LOG_LEVEL_INFO, PSTR(logVoltage));

  Unit_ACMeasure._lastRead = millis();
  return true;
}

bool ACMeasureisConnected()
{
  Wire.beginTransmission(UNIT_ACMEASURE_ADDR);
  int rv = Wire.endTransmission();
  return rv == 0;
}

void ACMeasureDetect(void)
{
  if (!I2cSetDevice(UNIT_ACMEASURE_ADDR))
  {
    AddLog(LOG_LEVEL_ERROR, PSTR("Can NOT connect to Unit AC Measure"));
    return;
  }

  AddLog(LOG_LEVEL_INFO, PSTR("Checking AC Measure at 0x42"));

  if(ACMeasureisConnected())
  {
    I2cSetActiveFound(UNIT_ACMEASURE_ADDR, Unit_ACMeasure.name);
    Unit_ACMeasure.count = 1;
    AddLog(LOG_LEVEL_INFO, PSTR("Unit AC Measure is detected!"));
  }
  else
  {
    AddLog(LOG_LEVEL_ERROR, PSTR("Unit AC Measure is NOT detected!"));
  }
}

void ACMeasureShow(bool json)
{
  char voltage_chr[FLOATSZ];
  dtostrfd(Unit_ACMeasure.voltage, Settings->flag2.voltage_resolution, voltage_chr);
  char current_chr[FLOATSZ];
  dtostrfd(Unit_ACMeasure.current, Settings->flag2.current_resolution, current_chr);

  bool jsconflg = false;
  // i dont understand clearly about the AdcShowContinuation
  // so that may be has the bug at this point
  // read one more time to fix the bug 
  // or if not a bug >> successfully and congratulations on yourself
  if(json)
  {
    AdcShowContinuation(&jsconflg);
    ResponseAppend_P(PSTR("\"AC Measure\":{\"" D_JSON_VOLTAGE "\":%f,\"" D_JSON_CURRENT "\":%f}"),Unit_ACMeasure.voltage, Unit_ACMeasure.current);
  }
#ifdef USE_WEBSERVER
  else
  {
    WSContentSend_PD(HTTP_SNS_VOLTAGE, voltage_chr);
    WSContentSend_PD(HTTP_SNS_CURRENT, current_chr);
  }
#endif

  if(jsconflg){
    ResponseJsonEnd();
  }
}

void ACMeasureEverySecond(void)
{
  if(TasmotaGlobal.uptime & 1)
  {
    if(ACMeasureReadData())
    {
      AddLogMissed(Unit_ACMeasure.name, Unit_ACMeasure.valid);
    }
  }
}


bool Xsns118(uint32_t function)
{
  if(!I2cEnabled(XI2C_95))
  {
    return false;
  }

  bool result = false;
  if(FUNC_INIT == function)
  {
    ACMeasureDetect();
  }
  else if(Unit_ACMeasure.count)
  {
    switch(function)
    {
      case FUNC_EVERY_SECOND:
      ACMeasureEverySecond();
        //ACMeasureReadData();
        //AddLog(LOG_LEVEL_INFO, PSTR("AC MEASURE READ DATA"));
        break;
      case FUNC_JSON_APPEND:
        ACMeasureShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        ACMeasureShow(0);
        break;
#endif
    }
  }
  return result;
}

#endif 
#endif 