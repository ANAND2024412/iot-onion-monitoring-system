## Edge Node – ESP32

The edge node acts as a gateway between the Arduino end nodes and the cloud.

### Functions
- Receives sensor data from end nodes via BLE
- Performs basic data aggregation
- Sends data to server/cloud over WiFi

### Hardware Used
- ESP32 Development Board

### Communication
- BLE (HM-10 / ESP32 BLE)
- WiFi (HTTP/MQTT)
