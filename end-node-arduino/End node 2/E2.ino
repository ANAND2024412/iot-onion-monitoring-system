#include <SoftwareSerial.h>
#include <DHT11.h>
#include <ArduinoJson.h>

// Define software serial for HM-10 (TX: 6, RX: 5)
SoftwareSerial HM10(5, 6); // RX, TX

// DHT11 Sensor
DHT11 dht11(3);

// MQ2 Gas Sensor
#define MQ2pin A0
float gasValue;

// Flame Sensor
#define FlamePin A1
int flameValue;

// Buzzer
#define Buzzer 4

void setup() {
    Serial.begin(9600);  // Debug serial monitor
    HM10.begin(9600);    // HM-10 BLE communication

    pinMode(Buzzer, OUTPUT);
    digitalWrite(Buzzer, LOW);  // Buzzer OFF initially

    Serial.println("End Node 2: Fire & Gas Monitoring with JSON via HM-10 BLE");
}

void loop() {
    // Read DHT11 Sensor Data
    int temperature = dht11.readTemperature();
    int humidity = dht11.readHumidity();

    // Read MQ2 Gas Sensor
    gasValue = analogRead(MQ2pin);

    // Read Flame Sensor
    flameValue = analogRead(FlamePin);

    // Fire detection and buzzer control
    if (flameValue < 500) {  // Adjust threshold based on your sensor calibration
        Serial.println("🔥 Alert: Fire Detected! 🔥");
        digitalWrite(Buzzer, HIGH);
        delay(2000);
        digitalWrite(Buzzer, LOW);
    }

    // Create JSON payload
    StaticJsonDocument<200> doc;
    doc["node"] = 2;
    doc["temp"] = temperature;
    doc["hum"] = humidity;
    doc["gas"] = gasValue;
    doc["flame"] = flameValue;

    // Serialize JSON to string
    String jsonOutput;
    serializeJson(doc, jsonOutput);

    // Debug output to Serial Monitor
    Serial.print("Sending JSON: ");
    serializeJsonPretty(doc, Serial);
    Serial.println();

    // Send JSON data via HM-10 BLE module
    HM10.println(jsonOutput);

    delay(1000);  // Wait 1 second before next reading
}
