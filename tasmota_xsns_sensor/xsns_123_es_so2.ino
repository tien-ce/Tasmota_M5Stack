#ifdef USE_RS485
#ifdef USE_ES_SO2

#define XSNS_123 123
#define XRS485_21 21

struct ESSO2t
{
    bool valid = false;
    float so2_value;
    char name[4] = "SO2";
}ESSO2;

#define ESSO2_ADDRESS_ID 0x01
#define ESSO2_ADDRESS_SO2_CONCENTRATION 0x0000
#define ESSO2_FUNCTION_CODE 0x03
#define ESSO2_TIMEOUT 150

bool ESSO2isConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(ESSO2_ADDRESS_ID, ESSO2_FUNCTION_CODE ,ESSO2_ADDRESS_SO2_CONCENTRATION,1);

    uint32_t start_time = millis();
    uint32_t wait_until = millis() + ESSO2_TIMEOUT;

    while(!TimeReached(wait_until))
    {
        delay(1);
        if(RS485.Rs485Modbus -> ReceiveReady()) break;
    }
    if(TimeReached(wait_until) && !RS485.Rs485Modbus -> ReceiveReady()) return false;

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);

    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("ESSO2 has error %d"),error);
        return false;
    }
    else
    {
        if(buffer[0]  == ESSO2_ADDRESS_ID) return true;
    }
    return false;
}

void ESSO2Init(void)
{
    if(!RS485.active) return;
    ESSO2.valid = ESSO2isConnected();
    if(ESSO2.valid) Rs485SetActiveFound(ESSO2_ADDRESS_ID, ESSO2.name);
    AddLog(LOG_LEVEL_INFO, PSTR(ESSO2.valid ? "ESSO2 is connected" : "ESSO2 is not detected"));
}

void ESSO2ReadData(void)
{
    if(!ESSO2.valid) return;

    if(isWaitingResponse(ESSO2_ADDRESS_ID)) return;

    if((RS485.requestSent[ESSO2_ADDRESS_ID] == 0) && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus->Send(ESSO2_ADDRESS_ID, ESSO2_FUNCTION_CODE, ESSO2_ADDRESS_SO2_CONCENTRATION, 1);
        RS485.requestSent[ESSO2_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[ESSO2_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

        if (error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus ESSO2 error: %d"), error);
        }
        else
        {
            uint16_t so2_valueRaw = (buffer[3] << 8) | buffer[4];
            ESSO2.so2_value = so2_valueRaw;
        }
        RS485.requestSent[ESSO2_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

const char HTTP_SNS_ESSO2[] PROGMEM = "{s} SO2 concentration {m} %.1f";
#define D_JSON_ESSO2 "ESSO2"

void ESSO2Show(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), ESSO2.name);
        ResponseAppend_P(PSTR("\"" D_JSON_ESSO2 "\":%.1f"), ESSO2.so2_value);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_ESSO2, ESSO2.so2_value);
    }
#endif
}

bool Xsns123(uint32_t function)
{
    if(!Rs485Enabled(XRS485_21)) return false;
    bool result = false;
    if(FUNC_INIT == function)
    {
        ESSO2Init();
    }
    else if(ESSO2.valid)
    {
        switch(function)
        {
            case FUNC_EVERY_250_MSECOND:
                ESSO2ReadData();
                break;
            case FUNC_JSON_APPEND:
                ESSO2Show(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                ESSO2Show(0);
                break;
#endif 
        }
    }
    return result;
}

#endif
#endif