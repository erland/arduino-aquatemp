#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NoDelay.h>
#include "arduino_secrets.h"

#define ONE_WIRE_BUS D2       
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire); 
uint8_t outsideSensor[8] = SECRET_OUTSIDE_SENSOR;
float outsideOffset = SECRET_OUTSIDE_SENSOR_ADJUSTMENT;
uint8_t insideSensor[8] = SECRET_INSIDE_SENSOR;
float insideOffset = -SECRET_INSIDE_SENSOR_ADJUSTMENT;

// Interval between WiFi network connection checks
noDelay networkingTime(5000);
noDelay sensorTime(10000);

ESP8266WebServer server(80);

void setup(void)
{
  Serial.begin(9600); 
  
  // Establish WiFi connection
  ensureNetworkConnection();

  // Initialize sensors
  sensors.begin();
  sensors.setWaitForConversion(true);

  // Setup web server
  server.on("/", handle_Request);
  server.begin();
}

void loop(void)
{ 
  // Check WiFi connection and re-establish connection if not connected
  if (networkingTime.update()) {
    ensureNetworkConnection();
  }

  server.handleClient();

  if (Serial && sensorTime.update()) {
  // Get temperatures
    Serial.println("Requesting temperatures: ");
    sensors.requestTemperatures();
    Serial.print("Number of devices: ");
    Serial.println(sensors.getDeviceCount());
    Serial.println("Temperature is: ");
    for(int i=0;i<sensors.getDeviceCount();i++) {
      DeviceAddress address;
      sensors.getAddress(address, i);
      printAddress(address);
      Serial.print(" = ");
      Serial.println(sensors.getTempCByIndex(i));
    }
  }
}

void handle_Request() {
  char response[40];
  sensors.requestTemperatures();
  server.send(200, "application/json", SendServerResponse(response, "tempInside",sensors.getTempC(insideSensor)+insideOffset, "tempOutside", sensors.getTempC(outsideSensor)+outsideOffset)); 
}

char* SendServerResponse(char* response, char* name1, float temperature1, char* name2, float temperature2){
  sprintf(response, "{\"%s\": %0.1f,\"%s\": %0.1f}", name1, temperature1, name2, temperature2);
  return response;
}

// Ensure network is connected and connect if it isn't
int wifiStatus = WL_IDLE_STATUS;
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
void ensureNetworkConnection() {
  wifiStatus = WiFi.status();
  if( wifiStatus != WL_CONNECTED ) {
    while (wifiStatus != WL_CONNECTED) {
      Serial.print("Attempting to connect to network:");
      Serial.println(ssid);
      wifiStatus = WiFi.begin(ssid, pass);
  
      int i=0;
      while (wifiStatus != WL_CONNECTED && i<10) {
        delay(1000);
        wifiStatus = WiFi.status();
        i++;
      }
    }
    Serial.print("You're connected to the network: ");
    Serial.print(WiFi.SSID());
    Serial.print(" with IP: ");
    Serial.println(WiFi.localIP());
  }
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++)
  {
          Serial.print(" ");
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
