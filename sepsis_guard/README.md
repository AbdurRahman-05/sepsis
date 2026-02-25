# SepsisGuard AI - ESP8266 Firmware

This folder contains the complete firmware for the NodeMCU ESP8266 to continuously monitor the patient and send the vital signs to the backend server.

## Hardware Setup
The system uses the NodeMCU ESP8266 (ESP-12E) to gather data from 4 different modules over I2C, OneWire, and Analog pins.

### Complete Wiring Table

| ESP8266 Pin | Component | Component Pin | Notes |
| :--- | :--- | :--- | :--- |
| **3V3** | MAX30102 | VIN | Power for MAX30102 |
| **3V3** | OLED SH1106 | VCC | Power for OLED Screen |
| **3V3** | DS18B20 | VCC | Power for Temp Sensor (Use 4.7kΩ pull-up resistor to D4) |
| **3V3** | AD8232 (ECG) | 3.3V | Power for ECG Module |
| **GND** | MAX30102 | GND | Common Ground |
| **GND** | OLED SH1106 | GND | Common Ground |
| **GND** | DS18B20 | GND | Common Ground |
| **GND** | AD8232 (ECG) | GND | Common Ground |
| **D1 (GPIO5)** | MAX30102 | SCL | I2C Clock |
| **D2 (GPIO4)** | MAX30102 | SDA | I2C Data |
| **D7 (GPIO13)**| OLED SH1106 | D1 (MOSI) | SPI Data |
| **D5 (GPIO14)**| OLED SH1106 | D0 (CLK) | SPI Clock |
| **RX (GPIO3)** | OLED SH1106 | DC | Data/Command |
| **D8 (GPIO15)**| OLED SH1106 | CS | Chip Select |
| **D3 (GPIO0)** | OLED SH1106 | RES | Reset |
| **D4 (GPIO2)** | DS18B20 | DQ / Data | OneWire Data (Requires 4.7kΩ pull-up to 3V3) |
| **D6 (GPIO12)**| AD8232 (ECG) | LO+ | Leads Off Detection Plus |
| **D0 (GPIO16)**| AD8232 (ECG) | LO- | Leads Off Detection Minus |
| **A0 (ADC0)**  | AD8232 (ECG) | OUTPUT | Analog ECG Signal |
| **SD3 (GPIO10)**| Buzzer | POSITIVE | Buzzer trigger pin |

> **Note on RX Pin usage:** The OLED `DC` pin is mapped to the `RX` pin (GPIO3) of the ESP8266 to avoid conflicts with I2C `D2`. This means you cannot use the RX pin for serial communication while the OLED is running, which is fine since we only use `Serial.print` (TX). 

> **Note on DS18B20:** You **must** wire a 4.7k ohm resistor between the Data pin (D4) and VCC (3V3) to act as a pull-up resistor. Without this, the temperature reading will fail.


## Arduino IDE Setup
In your Arduino IDE Library Manager, install the following:
1. `ArduinoJson` by Benoit Blanchon
2. `Adafruit GFX Library` by Adafruit
3. `Adafruit SH110X` by Adafruit
4. `OneWire` by Paul Stoffregen
5. `DallasTemperature` by Miles Burton
6. `SparkFun MAX3010x Pulse and Proximity Sensor Library` by SparkFun

## Code configuration
Before uploading out the code:
1. Update `WIFI_SSID` and `WIFI_PASSWORD`
2. Update `SERVER_URL` to point to your backend deployed URL or local IPv4.
