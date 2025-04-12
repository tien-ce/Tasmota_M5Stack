/*
  xsns_128_unit_4relay.ino - Unit 4 Relay of M5 support for Tasmota

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
#ifdef USE_4RELAY 
/*********************************************************************************************\
 * Unit module 4 relay 
 *
 * This driver supports the following sensors:
 * - UNIT_4_RELAY (address: 0x26)
\*********************************************************************************************/

#define XSNS_128 128
#define XI2C_94 94 // New define in I2CDevices.md

#define UNIT_4RELAY_ADDR      0x26
#define UNIT_4RELAY_REG       0x10
#define UNIT_4RELAY_RELAY_REG 0x11

struct UNIT_4RELAY
{
    char name[13] = "UNIT_4_RELAY";
    uint8_t count = 0;
} Unit_4Relay;

/*! @brief Write a certain length of data to the specified register address. */
void write1Byte(uint8_t address, uint8_t Register_address, uint8_t data)
{
    Wire.beginTransmission(address);
    Wire.write(Register_address);
    Wire.write(data);
    Wire.endTransmission();
}

/*! @brief Setting the mode of the device, and turn off all relays.
 *  @param mode Async = 0, Sync = 1. */
void Init(bool mode)
{
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_REG, mode);
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG, 0);
}

/*! @brief Setting the mode of the device.
 *  @param mode Async = 0, Sync = 1. */
void switchMode(bool mode)
{
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_REG, mode);
}

/*! @brief Read a certain length of data to the specified register address. */
uint8_t read1Byte(uint8_t address, uint8_t Register_address)
{
    Wire.beginTransmission(address); // Initialize the Tx buffer
    Wire.write(Register_address);   // Put slave register address in Tx buffer
    Wire.endTransmission();
    Wire.requestFrom(address, uint8_t(1));
    uint8_t data = Wire.read();
    return data;
}

/*! @brief Set the mode of all relays at the same time.
 *  @param state OFF = 0, ON = 1. */
void relayAll(bool state)
{
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG, state * (0x0f));
}

/*! @brief Set the mode of all leds at the same time.
 *  @param state OFF = 0, ON = 1. */
void ledAll(bool state)
{
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG, state * (0xf0));
}

/*! @brief Control the on/off of the specified relay.
 *  @param number Bit number of relay (0~3).
    @param state OFF = 0, ON = 1 . */
void relayWrite(uint8_t number, bool state)
{
    uint8_t StateFromDevice = read1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG);
    if (state == 0)
    {
        StateFromDevice &= ~(0x01 << number);
    }
    else
    {
        StateFromDevice |= (0x01 << number);
    }
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG, StateFromDevice);
}

/*! @brief Control the on/off of the specified led.
 *  @param number Bit number of led (0~3).
    @param state OFF = 0, ON = 1 . */
void ledWrite(uint8_t number, bool state)
{
    uint8_t StateFromDevice = read1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG);
    if (state == 0)
    {
        StateFromDevice &= ~(UNIT_4RELAY_REG << number);
    }
    else
    {
        StateFromDevice |= (UNIT_4RELAY_REG << number);
    }
    write1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG, StateFromDevice);
}

bool Unit4Relay_isConnected()
{
    Wire.beginTransmission(UNIT_4RELAY_ADDR);
    int rv = Wire.endTransmission();
    return rv == 0;
}

void Unit4RelayDetect(void)
{
    if (!I2cSetDevice(UNIT_4RELAY_ADDR))
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("Can NOT connect to Unit 4 Relay"));
        return;
    }

    AddLog(LOG_LEVEL_INFO, PSTR("Checking Unit 4 Relay at 0x26"));

    if (Unit4Relay_isConnected())
    {
        I2cSetActiveFound(UNIT_4RELAY_ADDR, Unit_4Relay.name);
        Unit_4Relay.count = 1;
        AddLog(LOG_LEVEL_INFO, PSTR("Unit 4 Relay is detected !"));
    }
    else
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("Unit 4 Relay is NOT detected!"));
    }
}

#define D_CMND_SET_RELAY         "SetRelay"   // SetRelay number state
#define D_CMND_SET_LED           "SetLed"
#define D_CMND_RELAY_ALL         "RelayAll"
#define D_CMND_LED_ALL           "LedAll"
#define D_CMND_SWITCH_MODE_4UNIT "SwitchMode_4Unit"
#define D_CMND_SWITCH_RELAY      "SwitchRelay"

const char kRelayCommands[] PROGMEM = "|" // No prefix
    D_CMND_SET_RELAY    "|"
    D_CMND_SET_LED      "|"
    D_CMND_RELAY_ALL    "|"
    D_CMND_LED_ALL      "|"
    D_CMND_SWITCH_RELAY "|"
    D_CMND_SWITCH_MODE_4UNIT;

void (* const RelayCommand[])(void) PROGMEM = {
    &CmndSetRelay,
    &CmndSetLed,
    &CmndRelayAll,
    &CmndLedAll,
    &CmndSwitchRelay,
    &CmndSwitchMode_4Unit
};

void CmndSetRelay(void)
{
    if((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 4)){
        uint8_t number = (XdrvMailbox.data[0] - '0');
        bool state = (XdrvMailbox.data[2] - '0');
        relayWrite(number,state);

        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{\"SetRelay %d\": \"%s\" }"), number, state ? "ON" : "OFF");
        ResponseCmndChar(PSTR(json_response));
    }
    else 
    {
        ResponseCmndChar("Invalid Command");
    }
}

void CmndSetLed(void)
{
    if ((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 4))
    {
        uint8_t number = (XdrvMailbox.data[0] - '0');
        bool state = (XdrvMailbox.data[2] - '0');
        ledWrite(number, state);

        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{\"SetLed %d\": \"%s\" }"), number, state ? "ON" : "OFF");
        ResponseCmndChar(PSTR(json_response));
    }
    else
    {
        ResponseCmndChar("Invalid Command");
    }
}

void CmndRelayAll(void)
{
    if((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 2))
    {
        bool state = (XdrvMailbox.data[0] - '0');
        relayAll(state);
        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{\"RelayAll\": \"%d\" }"), state);
        ResponseCmndChar(PSTR(json_response));
    }
    else
    {
        ResponseCmndChar("Invalid Command");
    }
}

void CmndLedAll(void)
{
    if ((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 2))
    {
        bool state = (XdrvMailbox.data[0] - '0');
        ledAll(state);

        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{\"LedAll\": \"%d\" }"), state);
        ResponseCmndChar(PSTR(json_response));
    }
    else
    {
        ResponseCmndChar("Invalid Command");
    }
}

void CmndSwitchMode_4Unit(void)
{
    if ((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 2))
    {
        bool mode = (XdrvMailbox.data[0] - '0');
        switchMode(mode);

        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{\"RELAY_MODE\": \"%d\" }"), mode);

        char text_response[20];
        snprintf_P(text_response, sizeof(text_response), PSTR("RELAY_MODE %d"), mode);

        ResponseCmndChar(PSTR(json_response));
        AddLog(LOG_LEVEL_INFO, text_response);
    }
    else
    {
        ResponseCmndChar("Invalid Command");
    }
}

void CmndSwitchRelay(void)
{
    if((XdrvMailbox.data_len >= 1) && (XdrvMailbox.data_len < 2))
    {
        char index_relay = XdrvMailbox.data[0];
        uint8_t state = read1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG);
        bool bit_check = (state & (1 << (index_relay - '0' - 1)));

        uint8_t idx = XdrvMailbox.data[0] - '0' - 1;
        if(bit_check)
        {
            relayWrite(idx,0);
        }
        else
        {
            relayWrite(idx,1);
        }

        char json_response[50];
        snprintf_P(json_response, sizeof(json_response), PSTR("{RELAY%d:%s}"), index_relay - '0', bit_check ? "ON" : "OFF");

        /* char text_response[20];
        snprintf_P(text_response, sizeof(text_response), PSTR("RELAY%d %s"), index_relay - '0', bit_check ? "ON" : "OFF"); */

        ResponseCmndChar(PSTR(json_response));
        //AddLog(LOG_LEVEL_INFO, text_response);
    }
    else
    {
        ResponseCmndChar("Invalid command");
    }
}

void showButton()
{
    char stemp[33] = "";

    WSContentSend_P(PSTR("<div style='padding:0;' id='k1' name='k1'></div><div></div>"));
    WSContentSend_P(HTTP_TABLE100); // "<table style='width:100%%'>"
    WSContentSend_P(PSTR("<tr>"));

    uint32_t rows = 1;
    uint32_t cols = 4;
    uint32_t button_ptr = 0;
    for (uint32_t button_idx = 1; button_idx <= 4; button_idx++)
    {
        snprintf_P(stemp, sizeof(stemp), PSTR("Relay %d"), button_idx);
        WSContentSend_P(PSTR("<td style='width:%d%%; text-align:center;'>"), 100 / cols);
        WSContentSend_P(PSTR("<button id='relay%d' onclick='switchRelay(%d)' "
                             "style='background:#1fa3ec;color:white;border-radius:8px;'>%s</button>"),
                        button_idx, button_idx, stemp);
        WSContentSend_P(PSTR("</td>"));

        if (button_idx % cols == 0)
        {
            WSContentSend_P(PSTR("</tr><tr>"));
        }
    }
    WSContentSend_P(PSTR("</tr></table>"));
    WSContentSend_P(PSTR("<script>"));

    uint32_t max_devices = 4;
    uint8_t state = read1Byte(UNIT_4RELAY_ADDR, UNIT_4RELAY_RELAY_REG);
    for (uint32_t idx = 1; idx <= max_devices; idx++)
    {
        bool not_active = !(state & (1 << (idx - 1)));
        if (not_active)
        {
            WSContentSend_P(PSTR("document.getElementById('relay%d').style.background='#%06x';"), idx, WebColor(COL_BUTTON_OFF));
        }
    }
    // Function to send a relay switch command
    WSContentSend_P(PSTR(
        "function switchRelay(relay) {"
        " var btn = document.getElementById('relay' + relay);"
        " var newColor = (btn.style.background === 'rgb(31, 163, 236)') ? '#ff0000' : '#1fa3ec';"
        " btn.style.background = newColor;"
        " var xhr = new XMLHttpRequest();"
        " xhr.open('GET', '/cm?cmnd=SwitchRelay ' + relay, true);"
        " xhr.send();"
        "}"));

    WSContentSend_P(PSTR("</script>"));
}

bool Xsns128(uint32_t function)
{
    if(!I2cEnabled(XI2C_94))
    {
        return false;
    }
    bool result = false;
    if(FUNC_INIT == function)
    {
        Unit4RelayDetect();
        Init(1);

    }
    else if(Unit_4Relay.count)
    {
        switch (function)
        {
        case FUNC_EVERY_SECOND:
            /* code */
        break;
        case FUNC_COMMAND:
        result = DecodeCommand(kRelayCommands, RelayCommand);
        break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_ADD_MAIN_BUTTON:
        showButton();
        break;
#endif
}
    }
    return result;
}

#endif
#endif 