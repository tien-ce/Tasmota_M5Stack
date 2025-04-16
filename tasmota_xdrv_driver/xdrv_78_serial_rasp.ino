#include "my_user_config.h"
#ifdef USE_RASP_UART
#define XDRV_78 78
#include "ArduinoJson.h"
class Device_info
{
public:
    String device_name;
    uint8_t channel;  // kênh truyền
    uint8_t addrHigh; // Địa chỉ cao
    uint8_t addrLow;  // Địa chỉ thấp
    float longitude;  // Kinh độ
    float latitude;   // Vĩ độ
    Device_info()
    {
        this->device_name = "";
    }
    Device_info(String device_name)
    {
        this->device_name = device_name;
    }
    Device_info(String device_name, float longitude, float latitude)
    {
        this->device_name = device_name;
        this->longitude = longitude;
        this->latitude = latitude;
    }
    void setRaspName(String device_name)
    {
        this->device_name = device_name;
    }
};
/* ********************************Biến toàn cục***********************************/
StaticJsonDocument<256> Rasp_mapping;
JsonObject Rasp_mapping_ojb = Rasp_mapping.to<JsonObject>();
/************************************************************************************* */
Device_info device_info("Device A", 106.76940000, 10.90682000);
bool init_success = false;
void RaspSerialInit(void)
{
    String mac_str = TasmotaGlobal.mqtt_client; // 		DVES_18E8AC
    // mac_str = WiFi.macAddress(); // "AB:CD:EF:GH:JK:LM"
    int first = mac_str.lastIndexOf("_"); // At _18....
    String mac_device = mac_str.substring(first+1); //"18E8AC"
    device_info.setRaspName(mac_device);
    if(PinUsed(3) || PinUsed(1)) // Nếu UART0 bị sử dụng
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("UART 0 is used"));
        return;
    }
    else
    {
        Serial.begin(9600);
        AddLog(LOG_LEVEL_INFO, PSTR("INIT SUCCESS"));
        init_success = true;
    }
}
#define D_CMND_SEND_Rasp_SERIAL "SendRasp"
#define D_CMND_SEND_Rasp_TELEMETRY "SendRaspTelemetry"
#define D_CMND_SEND_Rasp_ATTRIBUTE "SendRaspAttribute"
#define D_CMND_PRINT_MAPPING_Rasp "PrintMappingRasp"
const char kRaspSerialCommands[] PROGMEM = "|"
    D_CMND_SEND_Rasp_SERIAL "|"
    D_CMND_SEND_Rasp_TELEMETRY  "|"
    D_CMND_SEND_Rasp_ATTRIBUTE "|"
    D_CMND_PRINT_MAPPING_Rasp;

void (* const RaspSerialCommand[])(void) PROGMEM = {
    &CmndSendRasp,
    &CmndSendRaspTelemetry,
    &CmndSendRaspAttribute,
    &CmndPrintMapRasp
};
void CmndSendRaspAttribute(void){
    StaticJsonDocument<256> doc;
    JsonObject deviceA = doc.createNestedObject(device_info.device_name);
    deviceA["addrHigh"] = device_info.addrHigh;
    deviceA["addrLow"] = device_info.addrLow;
    deviceA["longitude"] = device_info.longitude;
    deviceA["latitude"] = device_info.latitude;
    String json_string;
    serializeJson(doc, json_string);
    AddLog(LOG_LEVEL_INFO, PSTR("Rasp Send: %s"), json_string.c_str());
    Serial.println(json_string);
    ResponseCmndDone();
}
void CmndSendRaspTelemetry(void)
{
    if (XdrvMailbox.data_len == 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("No data to transmit"));
        ResponseCmndDone();
        return;
    }

    StaticJsonDocument<256> doc;
    JsonArray arr = doc.createNestedArray(String(device_info.device_name));
    JsonObject data = arr.createNestedObject();

    StaticJsonDocument<128> mailboxDoc;
    DeserializationError error = deserializeJson(mailboxDoc, XdrvMailbox.data);
    if (error)
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("JSON parse error"));
        ResponseCmndDone();
        return;
    }
    for (JsonPair p : mailboxDoc.as<JsonObject>())
    {
        data[p.key()] = p.value();
    }
    String json_string;
    serializeJson(doc, json_string);
    Serial.println(json_string);
    ResponseCmndDone();
}
void CmndPrintMapRasp(void) {
    AddLog(LOG_LEVEL_INFO, PSTR("===== Rasp MAPPING ====="));
    
    for (JsonPair p : Rasp_mapping_ojb) {
        const char* key = p.key().c_str();     // "V1", "V2", ...
        const char* value = p.value().as<const char*>();  // "DHT11-Temperature", ...
        AddLog(LOG_LEVEL_INFO, PSTR("%s => %s"), key, value);
    }

    AddLog(LOG_LEVEL_INFO, PSTR("========================="));
}
void CmndSendRasp(void)
{
    if (XdrvMailbox.data_len == 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Nothing to transmit"));
        ResponseCmndDone();
        return;
    }
    char *tran = XdrvMailbox.data;
    AddLog(LOG_LEVEL_INFO, PSTR("Transmit data: %s"), tran);
    ResponseCmndDone();
}

void RaspSerial_COLLECT_DATA() {
    ResponseClear();
    XsnsCall(FUNC_JSON_APPEND);
    const char* raw = ResponseData();
    //  Bắt lỗi dấu phẩy ở đầu
    String fixed = raw;
    if (fixed.startsWith(",")) {
        fixed = fixed.substring(1);  // Bỏ dấu phẩy đầu
    }
    fixed = "{" + fixed + "}";       // Bọc thành JSON hoàn chỉnh
  
    AddLog(LOG_LEVEL_INFO, PSTR("Sensor JSON fixed: %s"), fixed.c_str());
  
    // Parse JSON
    StaticJsonDocument<256> input_doc;
    DeserializationError err = deserializeJson(input_doc, fixed);
    if (err) {
        AddLog(LOG_LEVEL_ERROR, PSTR("Sensor JSON parse error: %s"), err.c_str());
        return;
    }
  
    // Gói lại JSON kiểu {"Device":[{...}]}
    StaticJsonDocument<512> out_doc;
    JsonArray arr = out_doc.createNestedArray(String(device_info.device_name));
    JsonObject data = arr.createNestedObject();
    int v_count = 1;  // Đếm số biến V
    for (JsonPair p : input_doc.as<JsonObject>()) {
        const char* sensorName = p.key().c_str(); // "DHT11"
        JsonObject inner = p.value().as<JsonObject>();
        for(JsonPair q: inner){
            String sensor_sub = String(sensorName) + "-" + q.key().c_str();
             // Tạo key dạng V1, V2, ...
            String v_key = "V" + String(v_count);
            Rasp_mapping_ojb[v_key] = sensor_sub;
            data[v_key] = q.value();
            v_count++;
        }
        
    }
    String final_payload;
    serializeJson(out_doc, final_payload);
    Serial.println(final_payload);
    AddLog(LOG_LEVEL_INFO, PSTR("[FORWARD] Sent to Rasp via Serial (3/1): %s"), final_payload.c_str());
}
bool Xdrv78(uint32_t function)
{
    bool result = false;
    if(FUNC_PRE_INIT == function)
    {
        RaspSerialInit();
    }
    else if(init_success)
    {
        switch(function)
        {
            case FUNC_COMMAND:
                result = DecodeCommand(kRaspSerialCommands,RaspSerialCommand);
                break;
            case FUNC_AFTER_TELEPERIOD:
                RaspSerial_COLLECT_DATA();  
            break;
        }
    }
    return result;
}
#endif // USE_Rasp_UART
