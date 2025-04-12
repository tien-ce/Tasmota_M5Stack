/*
  xsns_128_dht20.ino - DHT20 temperature and humidity sensor support for Tasmota

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
  This library is built based on https://github.com/RobTillaart/DHT20.git
  All functions of this library are used for educational purposes.
*/

#ifdef USE_I2C
#ifdef USE_DHT20
/*********************************************************************************************\
 * Sensirion I2C temperature and humidity sensors
 *
 * This driver supports the following sensors:
 * - DHT20 (address: 0x38)
\*********************************************************************************************/

#define XSNS_49 49 
#define XI2C_93 93 // New define in I2CDevices.md

#define DHT20_OK 0
#define DHT20_ERROR_CHECKSUM -10
#define DHT20_ERROR_CONNECT -11
#define DHT20_MISSING_BYTES -12
#define DHT20_ERROR_BYTES_ALL_ZERO -13
#define DHT20_ERROR_READ_TIMEOUT -14
#define DHT20_ERRO_LASTREAD -15

#define DHT20_ADDR 0x38


struct DHT20
{
  float temperature = NAN;
  float humidity = NAN;
  float humOffset = NAN;
  float tempOffset = NAN;
  char name[6] = "DHT20";

  uint8_t count = 0;
  uint8_t valid = 0;
  uint8_t status = 0;
  uint32_t _lastRequest = 0;
  uint32_t _lastRead = 0;
  uint8_t bits[7];

} Dht20;

int Dht20RequestData();
int Dht20ReadData();
bool Dht20Read();
bool Dht20Convert();

////////////////////////////////////////////////
//
//  STATUS
//

uint8_t Dht20ReadStatus()
{
  Wire.requestFrom(DHT20_ADDR, (uint8_t)1);
  delay(1);
  return (uint8_t)Wire.read();
}

bool Dht20isCalibrated()
{
  return (Dht20ReadStatus() & 0x08) == 0x08;
}

bool Dht20isMeasuring()
{
  return (Dht20ReadStatus() & 0x80) == 0x80;
}

bool Dht20isIdle()
{
  return (Dht20ReadStatus() & 0x80) == 0x00;
}

uint8_t Dht20ComputeCrc(uint8_t data[], uint8_t len)
{
  // Compute CRC as per datasheet
  uint8_t crc = 0xFF;

  for (uint8_t x = 0; x < len; x++)
  {
    crc ^= data[x];
    for (uint8_t i = 0; i < 8; i++)
    {
      if (crc & 0x80)
      {
        crc = (crc << 1) ^ 0x31;
      }
      else
      {
        crc <<= 1;
      }
    }
  }
  return crc;
}

bool Dht20isConnected()
{
  Wire.beginTransmission(DHT20_ADDR);
  int rv = Wire.endTransmission();
  return rv == 0;
}

bool Dht20Begin()
{
  return Dht20isConnected();
}

///////////////////////////////////////////////////
//
// READ THE SENSOR
//

bool Dht20Read(void)
{
  if (Dht20.valid)
  {
    Dht20.valid--;
  }

  if (millis() - Dht20._lastRead < 1000)
  {
    return false;
  }

  int status = Dht20RequestData();
  if (status < 0)
    return status;
  // wait for measurement ready
  uint32_t start = millis();
  while (Dht20isMeasuring())
  {
    if (millis() - start >= 1000)
    {
      return false;
    }
    yield;
  }
  // read the measurement
  status = Dht20ReadData();
  if (status < 0)
    return false;

  Dht20.valid = SENSOR_MAX_MISS;

  // CONVERT AND STORE
  Dht20.status = Dht20.bits[0];
  uint32_t raw = Dht20.bits[1];
  raw <<= 8;
  raw += Dht20.bits[2];
  raw <<= 4;
  raw += (Dht20.bits[3] >> 4);
  Dht20.humidity = raw * 9.5367431640625e-5; // ==> / 1048576.0 * 100%;

  raw = (Dht20.bits[3] & 0x0F);
  raw <<= 8;
  raw += Dht20.bits[4];
  raw <<= 8;
  raw += Dht20.bits[5];
  Dht20.temperature = raw * 1.9073486328125e-4 - 50; //  ==> / 1048576.0 * 200 - 50;

  if (isnan(Dht20.temperature) || isnan(Dht20.humidity)) return false;

  // TEST CHECKSUM
  uint8_t crc = Dht20ComputeCrc(Dht20.bits, 6);
  if (crc != Dht20.bits[6]) return false;

  return true;
}

int Dht20RequestData()
{
  Wire.beginTransmission(DHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  int rv = Wire.endTransmission();

  Dht20._lastRequest = millis();
  return rv;
}

int Dht20ReadData()
{
  const uint8_t length = 7;
  int bytes = Wire.requestFrom(DHT20_ADDR, length);

  if (bytes == 0)
    return DHT20_ERROR_CONNECT;
  if (bytes < length)
    return DHT20_MISSING_BYTES;

  bool allZero = true;
  for (int i = 0; i < bytes; i++)
  {
    Dht20.bits[i] = Wire.read();
    allZero = allZero && (Dht20.bits[i] == 0);
  }

  if (allZero)
    return DHT20_ERROR_BYTES_ALL_ZERO;

  Dht20._lastRead = millis();
  return bytes;
}



////////////////////////////////////////////////
//
//  TEMPERATURE & HUMIDITY & OFFSET
//

void Dht20Detect(void)
{
  if (!I2cSetDevice(DHT20_ADDR))
    return;

  AddLog(LOG_LEVEL_INFO, PSTR("Checking DHT20 at 0x38"));

  if (Dht20isConnected()) // Directly check if sensor responds
  {
    I2cSetActiveFound(DHT20_ADDR, Dht20.name);
    Dht20.count = 1;
    AddLog(LOG_LEVEL_INFO, PSTR("DHT20 detected!"));
  }
  else
  {
    AddLog(LOG_LEVEL_ERROR, PSTR("DHT20 NOT detected!"));
  }
}

void Dht20Show(bool json)
{
  if (Dht20.valid)
  {
    TempHumDewShow(json, (0 == TasmotaGlobal.tele_period), Dht20.name, Dht20.temperature, Dht20.humidity);
  }
}

void Dht20EverySecond(void)
{
  if (TasmotaGlobal.uptime & 1)
  {
    if (!Dht20Read())
    {
      AddLogMissed(Dht20.name, Dht20.valid);
    }
  }
}

bool Xsns49(uint32_t function)
{
  if (!I2cEnabled(XI2C_93)){
    //AddLog(LOG_LEVEL_INFO, PSTR("DHT20"));
    return false;
  }

  bool result = false;
  if (FUNC_INIT == function)
  {
    Dht20Detect();
  }
  else if (Dht20.count)
  {
    switch (function)
    {
    case FUNC_EVERY_SECOND:
      Dht20EverySecond();
      break;
    case FUNC_JSON_APPEND:
      Dht20Show(1);
      break;
#ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
      Dht20Show(0);
      break;
#endif
    }
  }
  return result;
}

#endif // USE_DHT20
#endif // USE_I2C