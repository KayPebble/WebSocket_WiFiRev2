# WebSocket_WiFiRev2
Website w/ websockets for Arduino Uno WiFi rev2
Configurable to be an Access Point (IP 192.168.4.1) or connect to a wifi network (IP from DHCP).  Webpage available on port 80 will automatically connect to a websocket server on port 8080.

### Features:
- Control of LED_BUILTIN
- Control of RGB LED
- Realtime values from accelerometer , gyroscope, and temperature

### Libraries used:
- https://github.com/ocrdu/Arduino_LSM6DS3_T as the LSM6DS3 (IMU) library (Adds code for LSM6DS3 internal temperature sensor to https://github.com/arduino-libraries/Arduino_LSM6DS3);
