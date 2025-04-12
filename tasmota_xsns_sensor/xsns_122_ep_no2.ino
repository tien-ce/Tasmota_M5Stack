#ifdef USE_RS485
#ifdef USE_EP_NO2

#define XSNS_122 122
#define XRS485_23 23

struct EPNO2t
{
    bool valid = false;
    float no2_value;
    char name[4] = "NO2";
}EPNO2;

#define EPNO2_ADDRESS_ID 0x01
#define EPNO2_ADDRESS_NO2_CONCENTRATION 0x0000
#define EPNO2_FUNCTION_CODE 0x03
#define EPNO2_TIMEOUT 150

bool EPNO2isConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(EPNO2_ADDRESS_ID, EPNO2_FUNCTION_CODE, EPNO2_ADDRESS_NO2_CONCENTRATION, 1);

    uint32_t start_time = millis() ;
    uint32_t wait_until = millis() + EPNO2_TIMEOUT;
    
    while(!TimeReached(wait_until))
    {
        delay(1);
        if(RS485.Rs485Modbus -> ReceiveReady()) break;
        if(TimeReached(wait_until)) return false;
    }

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);
    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("EPNO2 has error %d"), error);
        return false;
    }
    else
    {
        if(buffer[0] == EPNO2_ADDRESS_ID) return true;
    }
    return false;
}

void EPNO2Init(void)
{
    if(!RS485.active) return;
    EPNO2.valid = EPNO2isConnected();
    if(EPNO2.valid) Rs485SetActiveFound(EPNO2_ADDRESS_ID, EPNO2.name);
    AddLog(LOG_LEVEL_INFO, PSTR(EPNO2.valid ? "EPNO2 is connected" : "EPNO2 is not detected"));
}

void EPNO2ReadData(void)
{
    if(!EPNO2.valid) return;
    
    if(isWaitingResponse(EPNO2_ADDRESS_ID)) return;

    if(RS485.requestSent[EPNO2_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(EPNO2_ADDRESS_ID, EPNO2_FUNCTION_CODE, EPNO2_ADDRESS_NO2_CONCENTRATION, 1);
        RS485.requestSent[EPNO2_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[EPNO2_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);

        if(error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus EPNO2 error: %d"),error);
        }
        else
        {
            uint16_t no2_valueRaw = (buffer[3] << 8) | buffer[4];
            EPNO2.no2_value = no2_valueRaw;
        }
        RS485.requestSent[EPNO2_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

const char HTTP_SNS_EPNO2[] PROGMEM = "{s} NO2 Concentration {m} %.1fppm";
#define D_JSON_EPNO2 "EPNO2"

void EPNO2Show(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EPNO2.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EPNO2 "\":%.1f"), EPNO2.no2_value);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EPNO2, EPNO2.no2_value);
    }
#endif
}

bool Xsns122(uint32_t function)
{
    if(!Rs485Enabled(XRS485_23)) return false;
    bool result = false;
    if(FUNC_INIT == function)
    {
        EPNO2Init();
    }
    else if(EPNO2.valid)
    {
        switch(function)
        {
            case FUNC_EVERY_250_MSECOND:
                EPNO2ReadData();
                break;
            case FUNC_JSON_APPEND:
                EPNO2Show(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                EPNO2Show(0);
                break;
#endif
        }
    }
    return result;
}

#endif 
#endif