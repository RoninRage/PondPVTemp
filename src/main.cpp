#include <Arduino.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncMqttClient.h>
#include "Credentials.h"

String hostname = "TeichTemp";

// MQTT Server Connection
#define MQTT_HOST IPAddress(192, 168, 1, 40)
#define MQTT_PORT 1883

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "teichtemp/temperature"
#define MQTT_PUB_BAT  "teichtemp/battery"
AsyncMqttClient mqttClient;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
String lastTemperature;

const int batLevelGPio = 35;
String batLevel;

//deep sleep related:
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  600        /* Time ESP32 will go to sleep (in seconds) */

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void connectToWifi() {
//  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
//  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
//  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
     /* Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());*/
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      //Serial.println("WiFi lost connection");
      break;
  }
}

double getTemperature(DallasTemperature sensor) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensor.requestTemperatures(); // Send the command to get temperature readings

  double t;
  int cnt = 0;
  do {
    t = sensor.getTempCByIndex(0); // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
    // 0 refers to the first IC on the wire
    delay(1);
    Serial.print(".");
    cnt++;
    if(cnt > 3) {
      return -127.0;
    }
  } while (t == 85.0 || t == -127.0);

  return t;
}

void ExecuteReadings() {  
    //delay(2500);
    sensors.requestTemperatures();
    // Temperature in Celsius degrees 
    //float temperature = sensors.getTempCByIndex(0);
    double temperature = getTemperature(sensors);
   // Serial.print(temperature);
  //  Serial.println(" *C");   
    lastTemperature = String(temperature);

    double batteryLevel = map(analogRead(batLevelGPio), 0.0f, 4095.0f, 0, 100);
   // Serial.print(batteryLevel);
    batLevel = String(batteryLevel);

    // Publish an MQTT message on topic esp32/dht/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, lastTemperature.c_str());
    // Publish an MQTT message on topic esp32/dht/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_BAT, 1, true, String(batLevel).c_str()); 

    delay(2500);
  //  Serial.println("Going to sleep now");
    WiFi.disconnect();    
    Serial.flush(); 
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
 //   Serial.println("This will never be printed");
}

void onMqttConnect(bool sessionPresent) {
 /* Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);*/
  ExecuteReadings();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
 // Serial.println("Disconnected from MQTT.");
}

void onMqttPublish(uint16_t packetId) {
 /* Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);*/
}

void setup() {
  delay(500);
  Serial.begin(115200);
  
  print_wakeup_reason();
 
  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();
}

void loop() {
  //nix hit
}