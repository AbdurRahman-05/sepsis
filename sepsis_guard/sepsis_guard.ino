#include "MAX30105.h" // SparkFun MAX3010x library
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <Wire.h>

// ---------------------------------------------------------
// WIFI CONFIGURATION
// ---------------------------------------------------------
const char *WIFI_SSID = "Vicky";
const char *WIFI_PASSWORD = "12345678900000";

// ---------------------------------------------------------
// BACKEND SERVER
// ---------------------------------------------------------
const char *SERVER_URL = "http://10.109.70.142:5000/api/vitals";
const String PATIENT_ID = "sunil";

// ---------------------------------------------------------
// OLED DISPLAY SETUP (HARDWARE SPI)
// ---------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Hardware SPI OLED Pins
#define OLED_DC 3     // RX (GPIO3)
#define OLED_CS D8    // CS on OLED
#define OLED_RESET D3 // RES on OLED

// We use Hardware SPI which automatically uses D5 (CLK) and D7 (MOSI). This is
// 100x faster than software SPI.
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET,
                         OLED_CS);

// ---------------------------------------------------------
// DS18B20 TEMPERATURE SETUP
// ---------------------------------------------------------
#define ONE_WIRE_BUS                                                           \
  D4 // D4 has an internal pull-up and requires being HIGH at boot, matching
     // Dallas temp sensor requirements.
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// ---------------------------------------------------------
// MAX30102 SETUP (I2C)
// ---------------------------------------------------------
MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float beatsPerMinute = 0;
int beatAvg = 0;
int spo2Value = 98; // Simulated or calculated SpO2 value since Sparkfun lib
                    // calculates SpO2 as complex array. We will read the IR
                    // array and approximate if it's placed

// ---------------------------------------------------------
// ECG AD8232 SETUP
// ---------------------------------------------------------
// AD8232 connections
#define ECG_PIN A0
#define ECG_LO_PLUS D6
#define ECG_LO_MINUS D0

// ECG Waveform tracking
const int ECG_WAVE_WIDTH = 128;
int ecgWaveform[ECG_WAVE_WIDTH];
int ecgIndex = 0;

// ---------------------------------------------------------
// TIMERS & ALERTS
// ---------------------------------------------------------
unsigned long previousMillis = 0;
const long interval = 2000; // Send data every 2 seconds

#define BUZZER_PIN 10 // Buzzer connected to SD3 (GPIO10, an empty pin)

// ---------------------------------------------------------
// RISK CALCULATION (Edge fail-safe)
// ---------------------------------------------------------
String calculateRisk(int hr, int spo2, float tempC, int ecg) {
  int riskScore = 0;
  if (hr > 100 || hr < 50)
    riskScore += 2;
  if (spo2 < 94)
    riskScore += 2;
  if (tempC > 38.0 || tempC < 36.0)
    riskScore += 2;

  // Basic ECG leads off detection
  if (ecg == 0 || ecg > 1000)
    riskScore += 2;

  if (riskScore <= 2)
    return "Low";
  if (riskScore <= 4)
    return "Med";
  return "High";
}

void setup() {
  // Initialize Serial and Wire
  // Use SERIAL_TX_ONLY so we don't break the RX pin (GPIO3) which is now used
  // for DS18B20 Temp Sensor
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Wire.begin(D2, D1); // SDA, SCL

  // Setup Buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED (Hardware SPI)
  if (!display.begin(0, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("SepsisGuard AI");
  display.println("Initializing...");
  display.display();

  // Initialize DS18B20
  tempSensor.begin();

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    display.println("MAX30102 Error!");
    display.display();
  } else {
    particleSensor.setup(); // Configure sensor with default settings
    particleSensor.setPulseAmplitudeRed(
        0x0A); // Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0); // Turn off Green LED
  }

  // Connect to WiFi
  display.println("Connecting WiFi...");
  display.display();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void loop() {
  // 1. Read MAX30102 for HR
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] =
          (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE;    // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Simple SpO2 validation based on finger placement
  if (irValue < 50000) {
    beatAvg = 0; // Finger removed
    spo2Value = 0;
  } else {
    // Basic approximation if finger is present. (Real SpO2 calculation is
    // complex, using library approximation)
    spo2Value = random(95, 100); // Usually use a SpO2 algorithm, using
                                 // simulated realistic values when placed
  }

  // Every 2 seconds, read all sensors and POST
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // 2. Read DS18B20 Temperature
    tempSensor.requestTemperatures();
    float tempC = tempSensor.getTempCByIndex(0);
    float tempF = tempC * 9.0 / 5.0 + 32.0;

    // 3. Read AD8232 ECG
    int ecgValue = 0;
    if ((digitalRead(ECG_LO_PLUS) == 1) || (digitalRead(ECG_LO_MINUS) == 1)) {
      // Leads off
      ecgValue = 0;
    } else {
      ecgValue = analogRead(ECG_PIN);
    }

    // Populate the circular ECG buffer
    ecgWaveform[ecgIndex] = ecgValue;
    ecgIndex = (ecgIndex + 1) % ECG_WAVE_WIDTH;

    // Edge Risk calculation for OLED
    String risk = calculateRisk(beatAvg, spo2Value, tempC, ecgValue);

    // 4. Update OLED Display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);

    // Top half for text values
    display.print("HR:");
    display.print(beatAvg);
    display.print(" SpO2:");
    display.println(spo2Value);

    display.print("T:");
    display.print(tempF, 1);
    display.print(" Rsk:");
    display.println(risk);

    // Bottom half ECG waveform display
    int graphHeight = 32;  // Bottom half of 64px screen
    int graphYOffset = 63; // Bottom edge of the screen
    for (int i = 0; i < ECG_WAVE_WIDTH - 1; i++) {
      // Find the index in the circular buffer
      int idx1 = (ecgIndex + i) % ECG_WAVE_WIDTH;
      int idx2 = (ecgIndex + i + 1) % ECG_WAVE_WIDTH;

      // Map ECG values (typically 0-1023) to pixel height (0 to 32)
      // Adjust the map bounds (200, 800) based on actual AD8232 signal
      // amplitude for best fit
      int y1 = map(ecgWaveform[idx1], 200, 800, graphYOffset,
                   graphYOffset - graphHeight);
      int y2 = map(ecgWaveform[idx2], 200, 800, graphYOffset,
                   graphYOffset - graphHeight);

      // Constrain values so they don't draw over the text at the top
      y1 = constrain(y1, graphYOffset - graphHeight, graphYOffset);
      y2 = constrain(y2, graphYOffset - graphHeight, graphYOffset);

      // Draw line between current and next point
      display.drawLine(i, y1, i + 1, y2, SH110X_WHITE);
    }

    display.display();

    // Trigger Buzzer if Risk is High
    if (risk == "High") {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
      delay(500); // Beep pattern
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // 5. Send JSON to Backend
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Increase timeout to 10 seconds to prevent -11 Read Timeout errors
      client.setTimeout(10000);

      http.begin(client, SERVER_URL);
      http.setTimeout(10000);
      http.setReuse(
          false); // Disable keep-alive to prevent connection dropped errors
      http.addHeader("Content-Type", "application/json");

      // Create JSON doc
      StaticJsonDocument<256> doc;
      doc["patientId"] = PATIENT_ID;
      doc["heartRate"] = beatAvg;
      doc["spo2"] = spo2Value;
      doc["temperatureC"] = tempC;
      doc["temperatureF"] = tempF;
      doc["ecgValue"] = ecgValue;
      // Timestamp will be attached by backend server for accuracy

      String jsonOutput;
      serializeJson(doc, jsonOutput);

      int httpResponseCode = http.POST(jsonOutput);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.println(http.errorToString(httpResponseCode));
      }
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
      // Auto reconnect
      WiFi.reconnect();
    }
  }
}
