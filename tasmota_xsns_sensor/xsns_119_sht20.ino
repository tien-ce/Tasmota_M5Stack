/*
  xsns_119_sht20.ino - SHT20 Sensor using Modbus support for Tasmota

  Copyright (C) 2023  Norbert Richter

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_SHT20
#ifdef USE_RS485

#define XSNS_119 119
#define XRS485_02 2
struct SHT20
{
    bool  valid = false;
    float temperature;
    float humidity;
    char  name[6] = "SHT20";
}Sht20;

#define SHT20_ADDRESS_ID 0x02
#define SHT20_ADDRESS_TEMP_AND_HUM 0x0001
#define SHT20_FUNCTION_CODE 0x04
#define SHT20_TIMEOUT 150

bool SHT20isConnected()
{
    if (!RS485.active)
        return false; // Return early if RS485 is not active

    RS485.Rs485Modbus->Send(SHT20_ADDRESS_ID, SHT20_FUNCTION_CODE, SHT20_ADDRESS_TEMP_AND_HUM, 1);

    uint32_t start_time = millis(); // Store start time
    /* while (!RS485.Rs485Modbus->ReceiveReady())
    {
        if (millis() - start_time > 200)
            return false; // Timeout after 200ms
        yield();          // Allow background tasks (important for ESP32)
    } */
    /* delay(150);
    if(!RS485.Rs485Modbus -> ReceiveReady()) return false; */
    uint32_t wait_until = millis() + SHT20_TIMEOUT;
    while(!TimeReached(wait_until))
    {
        delay(1);
        if(RS485.Rs485Modbus -> ReceiveReady()) break;
        if(TimeReached(wait_until)) return false;
    }

    uint8_t buffer[8];
    return (RS485.Rs485Modbus->ReceiveBuffer(buffer, 8) == 0 && buffer[0] == 0x02);
}

void SHT20Init()
{
    if (!RS485.active)
    {
        return;
    }

    Sht20.valid = SHT20isConnected();
    if(Sht20.valid) Rs485SetActiveFound(SHT20_ADDRESS_ID, Sht20.name);

    AddLog(LOG_LEVEL_INFO, PSTR(Sht20.valid ? "SHT20 is connected" : "SHT20 not detected"));
}

void SHT20ReadData(void)
{
    if(Sht20.valid == false) return;

    if(isWaitingResponse(SHT20_ADDRESS_ID)) return;

    if (RS485.requestSent[SHT20_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus->Send(SHT20_ADDRESS_ID, SHT20_FUNCTION_CODE, SHT20_ADDRESS_TEMP_AND_HUM, 0x0002);
        RS485.requestSent[SHT20_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }
    
    if (RS485.requestSent[SHT20_ADDRESS_ID] == 1 && millis() - RS485.lastRequestTime >= 200)
    {
        if (RS485.Rs485Modbus->ReceiveReady())
        {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);
            
            if (error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus SHT20 Error: %u"), error);
            }
            else if (buffer[0] == SHT20_ADDRESS_ID) // Ensure response is from the correct slave ID
            {
                uint16_t temperatureRaw = (buffer[3] << 8) | buffer[4];
                uint16_t humidityRaw = (buffer[5] << 8) | buffer[6];
                
                Sht20.temperature = temperatureRaw / 10.0;
                Sht20.humidity = humidityRaw / 10.0;
            }
            RS485.requestSent[SHT20_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

/* void SHT20ReadData(void)
{
    if(Sht20.valid == false)
    {
        current_sensor = 0;
        return;
    }

    if(current_sensor != 1) return;

    RS485.Rs485Modbus -> Send(0x02,0x04,0x0001,0x0002);
    delay(200);
    if(RS485.Rs485Modbus -> ReceiveReady())
    {
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);

        if(error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus SHT20 Error: %u"), error);
        }
        else if(buffer[0] == 0x02)
        {
            uint16_t temperatureRaw = (buffer[3] << 8) | buffer[4];
            uint16_t humidityRaw = (buffer[5] << 8) | buffer[6];

            Sht20.temperature = temperatureRaw / 10.0;
            Sht20.humidity = humidityRaw / 10.0;
        }
    }

    current_sensor = 0;
} */

const char HTTP_SNS_SHT20_TEMP[] PROGMEM = "{s} SHT20 Temperature {m}  %.1f °C";
const char HTTP_SNS_SHT20_HUMI[] PROGMEM = "{s} SHT20 Humidity {m}     %.1f %%";


void SHT20Show(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"),Sht20.name);
        ResponseAppend_P(PSTR("\"" D_JSON_TEMPERATURE "\":%*_f,\"" D_JSON_HUMIDITY "\":%*_f"),
                             Settings->flag2.temperature_resolution, &Sht20.temperature,
                             Settings->flag2.humidity_resolution, &Sht20.humidity);
        //ResponseAppend_P(PSTR("\"" D_JSON_WDS "\":%d"), wds.wind_direction);

        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_SHT20_TEMP, Sht20.temperature);
        WSContentSend_PD(HTTP_SNS_SHT20_HUMI, Sht20.humidity);
        //WSContentSend_PD(HTTP_SNS_WDS, wds.wind_direction);
        //WSContentSend_P("{s}%sTemperature {m} %.1f °C", Sht20.name, Sht20.temperature);
#endif
    }
}


bool Xsns119(uint32_t function)
{
    if(!Rs485Enabled(XRS485_02)) return false;
    
    bool result = false;
    if(FUNC_INIT == function)
    {
        SHT20Init();
    }
    else if(Sht20.valid)
    {
        switch(function)
        {
        case FUNC_EVERY_250_MSECOND:
        if(RS485.requestSent[0] == 1) break;
            SHT20ReadData();
            //ModbusPoll();
            break;
        case FUNC_JSON_APPEND:
            SHT20Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            SHT20Show(0);
            break;
#endif
        }
    }
    return result;
}
#endif
#endif