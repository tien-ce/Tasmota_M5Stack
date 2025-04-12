import json
import string

# Bảng map cho 10 giá trị telemetry: V1 ... V10
var telemetry_map = {
  "V1":   nil,
  "V2":   nil,
  "V3":   nil,
  "V4":   nil,
  "V5":   nil,
  "V6":   nil,
  "V7":   nil,
  "V8":   nil,
  "V9":   nil,
  "V10":  nil
}

#####################################
# Hàm đọc tất cả sensor từ Tasmota  #
#####################################
def getsensors()
  var sensors = json.load(tasmota.read_sensors())
  if sensors == nil || type(sensors) != 'instance'
    return nil
  end

  # Flatten tất cả (key:subkey -> value)
  var ressen = {}
  for entry: sensors.keys()
    if type(sensors[entry]) == 'instance'
      for subentry: sensors[entry].keys()
        ressen[entry + '-' + subentry] = sensors[entry][subentry]
      end
    end
  end

  if ressen.size() > 0
    return ressen
  else
    return nil
  end
end

#####################################
# Quản lý telemetry V1..V10         #
#####################################

# Thêm map: Vx -> sensorKey
def Add(vKey, sensorKey)
  # Nếu muốn linh hoạt, ta có thể ép "1" -> "V1", v.v. 
  # Ở đây ta giả sử đầu vào đã là "V1", "V2", ...
  var available_sensors = getsensors()
  if available_sensors == nil
    print("No sensors available; cannot add telemetry!")
    return false
  end

  # Kiểm tra sensorKey có trong list sensors không
  if available_sensors.find(sensorKey) != nil
    telemetry_map[vKey] = sensorKey
    print(f"Mapping {vKey} to sensor '{sensorKey}'")
  else
    print(f"Error: '{sensorKey}' not found in sensor list!")
    print("Available sensors: " + json.dump(available_sensors))
  end
end

#####################################
# Gửi data qua Lora (cron)          #
#####################################

def sendLoraBerry()
  var available_sensors = getsensors()
  if available_sensors == nil
    print("No sensors found, nothing to send!")
    return
  end

  var telemetry_out = {}
  var index = 1
  for key: available_sensors.keys()
    var vKey = "V" + str(index)
    telemetry_out[vKey] = available_sensors[key]
    index += 1
    if index > 10
      break
    end
  end

  var json_str = json.dump(telemetry_out)
  print("Sending LoRa Telemetry (auto V1–V10): " + json_str)
  tasmota.cmd("SendLoraTelemetry " + json_str)
end
# Tự động gọi mỗi 10 giây (có thể giữ hoặc bỏ tuỳ bạn)
tasmota.add_cron("*/10 * * * * *", sendLoraBerry, "every_10_seconds")