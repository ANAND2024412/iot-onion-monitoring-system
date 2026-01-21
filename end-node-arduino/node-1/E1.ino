#include <SoftwareSerial.h>
#include <DHT.h>
#include <HX711.h>
#include <ArduinoJson.h>

// BLE Module
SoftwareSerial HM10(5, 6); // RX, TX

// DHT11 Sensor
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ2 Sensor
#define MQ2pin A0
float sensorValue;

// HX711 Load Cell
#define DOUT 7
#define CLK 8
HX711 scale;
#define CALIBRATION_FACTOR -185000.0  // Calculated based on your values
float weight;

// Humidifier Relay
int relay = A2;

// Fan Control
#define FAN_ENA 9
#define FAN_IN1 10
#define FAN_IN2 11

void setup() {
  Serial.begin(9600);
  HM10.begin(9600);

  pinMode(relay, OUTPUT);
  pinMode(FAN_ENA, OUTPUT);
  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);

  digitalWrite(relay, HIGH); // Humidifier OFF
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  analogWrite(FAN_ENA, 0); // Fan OFF

  scale.begin(DOUT, CLK);
  scale.set_scale(CALIBRATION_FACTOR); 
  scale.tare(); // This sets the 88000 raw value as 0

  dht.begin();
  Serial.println("End Node 1 Initialized");
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);

  // Fan & Humidifier logic
  if (humidity < 65) {
    digitalWrite(relay, LOW); // Humidifier ON
    digitalWrite(FAN_IN1, HIGH);
    digitalWrite(FAN_IN2, LOW);
    analogWrite(FAN_ENA, 128); // Fan at half speed
  } else if (humidity >= 65 && humidity <= 69) {
    digitalWrite(relay, HIGH); // Humidifier OFF
    digitalWrite(FAN_IN1, LOW);
    digitalWrite(FAN_IN2, LOW);
    analogWrite(FAN_ENA, 0);   // Fan OFF
  } else {
    digitalWrite(relay, HIGH); // Humidifier OFF
    digitalWrite(FAN_IN1, HIGH);
    digitalWrite(FAN_IN2, LOW);
    analogWrite(FAN_ENA, 255); // Fan full speed
  }

  // Gas Reading
  sensorValue = analogRead(MQ2pin);
  Serial.print("Gas Reading: "); Serial.println(sensorValue);

  // Weight Reading
  weight = scale.get_units(5);
  Serial.print("Weight: "); Serial.print(weight, 3); Serial.println(" kg");

  // Send JSON via BLE
  StaticJsonDocument<256> doc;
  doc["node"] = 1;
  doc["temp"] = temperature;
  doc["hum"] = humidity;
  doc["gas"] = sensorValue;
  doc["weight"] = weight;

  String output;
  serializeJson(doc, output);
  HM10.println(output);

  delay(1000);
}
