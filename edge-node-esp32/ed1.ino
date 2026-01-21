#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi Credentials
const char* ssid = "Wifi";
const char* password = "1234567890";

// Server URL
const char* serverURL = "https://onion-server-g77l.onrender.com/api/update-sensors";

// BLE Node Addresses
NimBLEAddress node1Address("D4:F5:13:FA:EC:64", BLE_ADDR_PUBLIC);
NimBLEAddress node2Address("64:33:DB:96:1C:5D", BLE_ADDR_PUBLIC);

#define SERVICE_UUID         "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID  "0000ffe1-0000-1000-8000-00805f9b34fb"

// BLE clients and characteristics
NimBLEClient* pClient1 = nullptr;
NimBLEClient* pClient2 = nullptr;
NimBLERemoteCharacteristic* pChar1 = nullptr;
NimBLERemoteCharacteristic* pChar2 = nullptr;

String jsonBuffer1 = "";
String jsonBuffer2 = "";
const int FLAME_THRESHOLD = 500;

// Sensor data structure
struct SensorData {
  int warehouse_id;
  float temperature;
  float humidity;
  float methane;
  float weight;
  int flame;
  float avg_humidity;
};

// Queue for sensor data
#define MAX_QUEUE_SIZE 50
SensorData sendQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;

bool enqueueSensorData(const SensorData& data) {
  int nextTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  if (nextTail == queueHead) {
    Serial.println("⚠ Send queue full");
    return false;
  }
  sendQueue[queueTail] = data;
  queueTail = nextTail;
  return true;
}

bool dequeueSensorData(SensorData& data) {
  if (queueHead == queueTail) return false;
  data = sendQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  return true;
}

void sendToServer(const SensorData& data) {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["warehouse_id"] = data.warehouse_id;
  doc["temperature"] = data.temperature;
  doc["humidity"] = data.humidity;
  doc["methane"] = data.methane;
  doc["weight"] = data.weight;
  doc["flame"] = data.flame;
  doc["avg_humidity"] = data.avg_humidity;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  int httpResponseCode = http.POST(jsonPayload);
  Serial.print("📤 Sending data: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✅ Server: " + response);
  } else {
    Serial.println("❌ Error: " + http.errorToString(httpResponseCode));
  }
  http.end();
}

// Rolling humidity buffer
#define HUMIDITY_BUFFER_SIZE 5
float humidityBuffer[HUMIDITY_BUFFER_SIZE];
int humidityIndex = 0;
int humidityCount = 0;

float computeAvgHumidity(float newHumidity) {
  humidityBuffer[humidityIndex] = newHumidity;
  humidityIndex = (humidityIndex + 1) % HUMIDITY_BUFFER_SIZE;
  if (humidityCount < HUMIDITY_BUFFER_SIZE) humidityCount++;

  float sum = 0;
  for (int i = 0; i < humidityCount; i++) {
    sum += humidityBuffer[i];
  }

  return sum / humidityCount;
}

// 🟦 Fan and Humidifier Logic - Based on LATEST Node 1 Humidity
void controlFanHumidifier(float currentHumidity) {
  if (currentHumidity < 60.0) {
    Serial.println("💧 Humidity < 60% → Fan: Half Speed, Humidifier: ON");
  } else if (currentHumidity >= 60.0 && currentHumidity <= 69.0) {
    Serial.println("💨 Humidity 60–69% → Fan: OFF, Humidifier: OFF");
  } else if (currentHumidity > 70.0) {
    Serial.println("🔥 Humidity > 70% → Fan: Full Speed, Humidifier: OFF");
  }
}

// Node 1 BLE Callback
void notifyCallbackNode1(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  for (size_t i = 0; i < length; i++) {
    jsonBuffer1 += (char)pData[i];
    if (pData[i] == '}') {
      Serial.println("📡 Node1: " + jsonBuffer1);
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, jsonBuffer1);
      if (!error) {
        float hum = doc["hum"];
        float avgHum = computeAvgHumidity(hum);
        controlFanHumidifier(hum); // Use current humidity for logic

        SensorData data = {
          .warehouse_id = 1,
          .temperature = doc["temp"],
          .humidity = hum,
          .methane = doc["gas"],
          .weight = doc["weight"],
          .flame = 0,
          .avg_humidity = avgHum
        };
        if (!enqueueSensorData(data)) Serial.println("⚠ Queue full");
      } else {
        Serial.print("❌ Node1 JSON: ");
        Serial.println(error.c_str());
      }
      jsonBuffer1 = "";
    }
  }
}

// Node 2 BLE Callback
void notifyCallbackNode2(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  for (size_t i = 0; i < length; i++) {
    jsonBuffer2 += (char)pData[i];
    if (pData[i] == '}') {
      Serial.println("📡 Node2: " + jsonBuffer2);
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, jsonBuffer2);
      if (!error) {
        int flame = doc["flame"];
        if (flame < FLAME_THRESHOLD) {
          Serial.println("🔥 FIRE DETECTED!");
        }

        SensorData data = {
          .warehouse_id = 2,
          .temperature = doc["temp"],
          .humidity = doc["hum"],
          .methane = doc["gas"],
          .weight = doc["weight"],
          .flame = flame,
          .avg_humidity = computeAvgHumidity(doc["hum"]) // Just for visual
        };
        if (!enqueueSensorData(data)) Serial.println("⚠ Queue full");
      } else {
        Serial.print("❌ Node2 JSON: ");
        Serial.println(error.c_str());
      }
      jsonBuffer2 = "";
    }
  }
}

bool connectToNode(NimBLEAddress address, NimBLEClient*& client,
                   NimBLERemoteCharacteristic*& pChar,
                   void (callback)(NimBLERemoteCharacteristic*, uint8_t*, size_t, bool)) {
  Serial.print("🔗 Connecting to ");
  Serial.println(address.toString().c_str());

  client = NimBLEDevice::createClient();
  if (!client->connect(address)) {
    Serial.println("❌ Connection failed");
    return false;
  }

  NimBLERemoteService* service = client->getService(SERVICE_UUID);
  if (!service) {
    Serial.println("❌ Service not found");
    client->disconnect();
    return false;
  }

  pChar = service->getCharacteristic(CHARACTERISTIC_UUID);
  if (!pChar) {
    Serial.println("❌ Characteristic not found");
    client->disconnect();
    return false;
  }

  if (pChar->canNotify()) {
    if (!pChar->subscribe(true, callback)) {
      Serial.println("❌ Subscribe failed");
      client->disconnect();
      return false;
    }
  } else {
    Serial.println("⚠ No notifications");
    client->disconnect();
    return false;
  }

  Serial.println("✅ Connected");
  return true;
}

void setup() {
  Serial.begin(115200);
  NimBLEDevice::init("EdgeNode");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print("...");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed");
  }

  // Connect to BLE nodes
  if (connectToNode(node1Address, pClient1, pChar1, notifyCallbackNode1)) {
    Serial.println("✅ Node1 Connected");
  }
  if (connectToNode(node2Address, pClient2, pChar2, notifyCallbackNode2)) {
    Serial.println("✅ Node2 Connected");
  }
}

void loop() {
  SensorData data;
  if (dequeueSensorData(data)) {
    sendToServer(data);
    delay(100);
  } else {
    delay(50);
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 60000) {
    Serial.println("⌛ Checking status...");
    lastPrint = millis();
  }
}