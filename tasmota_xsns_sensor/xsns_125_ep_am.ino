#ifdef USE_RS485
#ifdef USE_EPAM

#define XSNS_125 125
#define XRS485_24 24

struct EPAMt
{
    bool valid = false;
    float PM2_5;
    float PM10;
    float PM1;
    char name[12] = "Air Quality";
}EPAM;

#define EPAM_ADDRESS_ID 0x24
#define EPAM_ADDRESS_PM_2_5 0x0004
#define EPAM_ADDRESS_PM10 0x0009
#define EPAM_ADDRESS_PM1 0x0000
#define EPAM_FUNCTION_CODE 0x03
#define EPAM_TIMEOUT 150

bool EPAMisConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus -> Send(EPAM_ADDRESS_ID, EPAM_FUNCTION_CODE, EPAM_ADDRESS_PM_2_5, 1);

    uint32_t start_time = millis();
    uint32_t wait_until = millis() + EPAM_TIMEOUT;

    while(!TimeReached(wait_until))
    {
        delay(1);
        if(RS485.Rs485Modbus -> ReceiveReady()) break;
        if(TimeReached(wait_until)) return false;
    }

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,8);

    if(error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("EPAM has error %d"), error);
        return false;
    }
    else
    {
        if(buffer[0] == EPAM_ADDRESS_ID) return true;
    }
    return false;
}

void EPAMInit(void)
{
    if(!RS485.active) return;
    EPAM.valid = EPAMisConnected();
    if(EPAM.valid) Rs485SetActiveFound(EPAM_ADDRESS_ID, EPAM.name);
    AddLog(LOG_LEVEL_INFO, PSTR(EPAM.valid ? "EPAM is connected" : "EPAM is not detected"));
}

void EPAMReadData(void)
{
    if(EPAM.valid == false) return;

    if(isWaitingResponse(EPAM_ADDRESS_ID)) return;

    static const struct 
    {
        uint16_t EPAMRegAddr;
        uint8_t EPAMRegCount;
    } EPAMModbusRequests[] = {
        {0x0004, 1}, // PM2.5
        {0x0009, 1}  // PM10
    };
    
    static uint8_t EPAMRequestIndex = 0;

    if(RS485.requestSent[EPAM_ADDRESS_ID] == 0 && RS485.lastRequestTime == 0)
    {
        RS485.Rs485Modbus -> Send(EPAM_ADDRESS_ID, EPAM_FUNCTION_CODE, EPAMModbusRequests[EPAMRequestIndex].EPAMRegAddr, EPAMModbusRequests[EPAMRequestIndex].EPAMRegCount);
        RS485.requestSent[EPAM_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }  
    if((RS485.requestSent[EPAM_ADDRESS_ID] == 1) && millis() - RS485.lastRequestTime >= 200)
    {
        if(RS485.Rs485Modbus -> ReceiveReady())
        {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer, sizeof(buffer));

            if(error)
            {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus EPAM error %d"),error);
            }
            else if(buffer[0] == EPAM_ADDRESS_ID)
            {
                switch(EPAMRequestIndex)
                {
                    case 0:
                        EPAM.PM2_5 = ((buffer[3] << 8) | buffer[4]) / 100.0;
                    break;
                    case 1:
                        EPAM.PM10 = ((buffer[3] << 8) | buffer[4]) / 100.0;
                    
                    break;
                }
                //uint16_t pm1Raw = (buffer[7] << 8) | buffer[8];
            }
            EPAMRequestIndex = (EPAMRequestIndex + 1) % (sizeof(EPAMModbusRequests) / sizeof(EPAMModbusRequests[0]));
            RS485.requestSent[EPAM_ADDRESS_ID] = 0;
            RS485.lastRequestTime = 0;
        }
    }
}

const char HTTP_SNS_EPAM_PM2_5[] PROGMEM = "{s} EPAM PM2.5 {m} %.1fμg/m³";
const char HTTP_SNS_EPAM_PM10[] PROGMEM = "{s} EPAM PM10.0 {m} %.1fμg/m³";
//const char HTTP_SNS_EPAM_PM1[] PROGMEM = "{s} EPAM PM1.0 {m} %.1fμg/m³";

#define D_JSON_EPAM_PM2_5 "EPAM PM2.5"
#define D_JSON_EPAM_PM10 "EPAM PM10.0"
//#define D_JSON_EPAM_PM1 "EPAM PM1.0"

void EPAMShow(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EPAM.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EPAM_PM2_5 "\":%.1f,"), EPAM.PM2_5);
        ResponseAppend_P(PSTR("\"" D_JSON_EPAM_PM10 "\":%.1f"), EPAM.PM10);
        //ResponseAppend_P(PSTR("\"" D_JSON_EPAM_PM1 "\":%.1f"), EPAM.PM1);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EPAM_PM2_5, EPAM.PM2_5);
        WSContentSend_PD(HTTP_SNS_EPAM_PM10, EPAM.PM10);
        //WSContentSend_PD(HTTP_SNS_EPAM_PM1, EPAM.PM1);
    }
#endif
}

bool Xsns125(uint32_t function)
{
    if(!Rs485Enabled(XRS485_24)) return false;

    bool result = false;
    if(FUNC_INIT == function)
    {
        EPAMInit();
    }
    else if(EPAM.valid)
    {
        switch (function)
        {
            case FUNC_EVERY_250_MSECOND:
                EPAMReadData();
                break;
            case FUNC_JSON_APPEND:
                EPAMShow(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                EPAMShow(0);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif