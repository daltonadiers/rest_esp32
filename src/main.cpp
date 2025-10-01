#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "DHTesp.h"
#include <WiFiManager.h>

DHTesp dht;
#define  DHT11_PIN 33       

// Web server running on port 80
WebServer server(80);

float temperature;
float humidity;
int led1Status = 3;
bool led2Status = false;

// set the pin for the PWM LED
const int ledPinPWM = 23;  // 16 corresponds to GPIO16

// set the PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

// set the pin for the Boolean LED
const int ledPinBool = 22;  // 17 corresponds to GPIO17



unsigned long measureDelay = 3000;                //    NOT LESS THAN 2000!!!!!   
unsigned long lastTimeRan;

StaticJsonDocument<1024> jsonDocument;

char buffer[1024];

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  
  // Get RGB components
  led1Status = jsonDocument["led1Status"];
  led2Status = jsonDocument["led2Status"];

  // Respond to the client
  server.send(200, "application/json", "{}");
}

void createJson(char *name, float value, char *unit) {  
  jsonDocument.clear();
  jsonDocument["name"] = name;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);  
}
 
void addJsonObject(char *name, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["name"] = name;
  obj["value"] = value;
  obj["unit"] = unit; 
}


void getValues() {
  Serial.println("Get all values");
  jsonDocument.clear(); // Clear json buffer
  addJsonObject("temperature", temperature, "°C");
  addJsonObject("humidity", humidity, "%");
  addJsonObject("led1Status", led1Status, "%");
  addJsonObject("led2Status", led2Status, "boolean");

  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void setupApi() {
  server.on("/getValues", getValues);
  server.on("/setStatus", HTTP_POST, handlePost);
 
  // start server
  server.begin();
}





void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 


  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("DaltonEsp32","12345671"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("Connected...yeey :)");
  }

  dht.setup(DHT11_PIN, DHTesp::DHT11); // Connect DHT sensor to GPIO 14 

  setupApi();

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPinPWM, ledChannel);

  pinMode(ledPinBool, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();

    // measurements every measureDelay ms
  if (millis() > lastTimeRan + measureDelay)  {
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    lastTimeRan = millis();

   }

  ledcWrite(ledChannel, led1Status);


  if(led2Status) {
    digitalWrite(ledPinBool, HIGH);
  } else {
    digitalWrite(ledPinBool, LOW);
  } 


}