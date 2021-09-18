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

const int oneWireBus = 4; // GPIO where the DS18B20 is connected to
OneWire oneWire(oneWireBus); 
DallasTemperature sensors(&oneWire);
String lastTemperature;

const int batLevelGPio = 35;
String batLevel;

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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      break;
  }
}

double getTemperature(DallasTemperature sensor) {
  sensor.requestTemperatures(); // Send the command to get temperature readings

  double t;
  int cnt = 0;
  do {
    t = sensor.getTempCByIndex(0); // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
    delay(1);
    Serial.print(".");
    cnt++;
    if(cnt > 3) {
      return -127.0;
    }
  } while (t == 85.0 || t == -127.0);

  return t;
}

String calculate_Bat_Percentage(double bat_min,double bat_max, double voltage)
{
  return String(((voltage - bat_min) / (bat_max - bat_min)) * 100,2);
}

void ExecuteReadings() 
{
  sensors.requestTemperatures();    
  double temperature = getTemperature(sensors); // Temperature in Celsius degrees 
  lastTemperature = String(temperature);

  double batteryLevel = map(analogRead(batLevelGPio), 2931, 4095, 3, 4.2);
  batLevel = calculate_Bat_Percentage(3.6, 4.2, batteryLevel);
     
  mqttClient.publish(MQTT_PUB_TEMP, 1, true, lastTemperature.c_str());
  mqttClient.publish(MQTT_PUB_BAT, 1, true, String(batLevel).c_str()); 

  delay(2500);
  WiFi.disconnect();    
  Serial.flush(); 
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void onMqttConnect(bool sessionPresent) 
{
  ExecuteReadings();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) { 
}

void onMqttPublish(uint16_t packetId) { 
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