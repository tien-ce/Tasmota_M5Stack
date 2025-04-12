#ifdef USE_RS485
#ifdef USE_EP_O3

#define XSNS_124 124
#define XRS485_22 22
struct EPO3t
{
    bool valid = false;
    float o3_value;
    char name[3] = "O3";
}EPO3;

#define EPO3_ADDRESS_ID 0x01
#define EPO3_ADDRESS_O3_CONCENTRATION 0x0000
#define EPO3_FUNCTION_CODE 0x03
#define EPO3_TIMEOUT 150

bool EPO3isConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(EPO3_ADDRESS_ID, EPO3_FUNCTION_CODE, EPO3_ADDRESS_O3_CONCENTRATION, 1);

    uint32_t start_time = millis();
    uint32_t wait_until = millis() + EPO3_TIMEOUT;

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
        AddLog(LOG_LEVEL_INFO, PSTR("EPO3 has error %d"),error);
    }
    else
    {
        if(buffer[0] == EPO3_ADDRESS_ID) return true;
    }
    return false;
}

void EPO3Init(void)
{
    if(!RS485.active) return; 
    EPO3.valid = EPO3isConnected();
    if(EPO3.valid) Rs485SetActiveFound(EPO3_ADDRESS_ID, EPO3.name);
    AddLog(LOG_LEVEL_INFO, PSTR(EPO3.valid ? "EPO3 is connected" : "EPO3 is not detected"));
}

void EPO3ReadData(void)
{
    if(!EPO3.valid) return;

    if(isWaitingResponse(EPO3_ADDRESS_ID)) return;
    //AddLog(LOG_LEVEL_INFO, PSTR("Checkingg"));
    if((RS485.requestSent[EPO3_ADDRESS_ID] == 0) && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(EPO3_ADDRESS_ID, EPO3_FUNCTION_CODE, EPO3_ADDRESS_O3_CONCENTRATION, 1);
        RS485.requestSent[EPO3_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if((RS485.requestSent[EPO3_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, 8);

        if(error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus EPO3 error %d"), error);
        }
        else
        {
            uint16_t o3_valueRaw = (buffer[3] << 8) | buffer[4];
            EPO3.o3_value = o3_valueRaw;
            //AddLog(LOG_LEVEL_INFO, PSTR("ReadData O3 successful"));
        }
        RS485.requestSent[EPO3_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

const char HTTP_SNS_EPO3[] PROGMEM = "{s} O3 concentration {m} %.1fppm";
#define D_JSON_EPO3 "EPO3"

void EPO3Show(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EPO3.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EPO3 "\":%.1f"), EPO3.o3_value);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EPO3, EPO3.o3_value);
    }
#endif
}


bool Xsns124(uint32_t function)
{
    if(!Rs485Enabled(XRS485_22)) return false;
    bool result = false;
    if(FUNC_INIT == function)
    {
        EPO3Init();
    }
    else if(EPO3.valid)
    {
        switch(function)
        {
            case FUNC_EVERY_250_MSECOND:
                EPO3ReadData();
                break;
            case FUNC_JSON_APPEND:
                EPO3Show(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                EPO3Show(0);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif