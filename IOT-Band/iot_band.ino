#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
#include "Adafruit_GFX.h"

WidgetLED led1(V8);
#define switch1 D8
#define DHTTYPE DHT11
#include "DHT.h"
uint8_t DHTPin = 2;
DHT dht(DHTPin, DHTTYPE);


#define REPORTING_PERIOD_MS 5000

char auth[] = "otF8P6cDZCccbEjwwOMJd7VF2NpJnYku";  // Authentication Token Sent by Blynk
char ssid[] = "sachin s21";                        //WiFi SSID
char pass[] = "Seera143";                          //WiFi Password

const char serverName[] = "http://13.50.246.130/new/process-sensor-data";

// Connections : SCL PIN - D1 , SDA PIN - D2 , INT PIN - D0
PulseOximeter pox;

int uid = 123;
float BPM, SpO2, Temperature;
int isEmergencyButtonPressed = 0;
uint32_t tsLastReport = 0;


void onBeatDetected() {
  Serial.println("Beat Detected!");

  // Read temperature as Celsius (the default)
  Temperature = dht.readTemperature();

  if (isnan(Temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }

  Serial.print(F("Temperature:"));
  Serial.print(Temperature);
  Serial.print(F("°C"));

  Blynk.virtualWrite(V3, Temperature);

  if (!digitalRead(switch1)) {
    isEmergencyButtonPressed = 0;
    Serial.println("isEmergencyButtonPressed:");
    Serial.println(isEmergencyButtonPressed);
    led1.off();
  } else {
    isEmergencyButtonPressed = 1;
    Serial.println("isEmergencyButtonPressed:");
    Serial.println(isEmergencyButtonPressed);
    led1.on();
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(DHTPin, INPUT);
  pinMode(switch1, INPUT);
  dht.begin();
  pinMode(16, OUTPUT);
  Blynk.begin(auth, ssid, pass);


  WiFi.begin(ssid, pass);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());


  Serial.print("Initializing Pulse Oximeter..");
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
}


void loop() {
  pox.update();
  Blynk.run();

  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart rate:");
    Serial.print(BPM);
    Serial.print("SpO2:");
    Serial.print(SpO2);
    Serial.println("%");

    Blynk.virtualWrite(V5, BPM);
    Blynk.virtualWrite(V6, SpO2);

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");

      char jsonString[256];
      sprintf(jsonString, "{\"uid\":\"%d\",\"heartRate\":\"%f\",\"oxygenSaturation\":\"%f\",\"temperature\":\"%f\",\"isEmergencyButtonPressed\":\"%d\"}", uid, BPM, SpO2, Temperature, isEmergencyButtonPressed);
      int httpResponseCode = http.POST(jsonString);

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }

    tsLastReport = millis();
  }
}