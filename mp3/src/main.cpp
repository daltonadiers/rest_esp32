#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#define RXD2 16
#define TXD2 17
#define POT_PIN 34  

WebServer server(80);
StaticJsonDocument<1024> jsonDocument;

char buffer[1024];
const int quantLeds = 6;
int ledPins[quantLeds] = {2, 4, 18, 19, 21, 23};

void setupApi();
void playRebolado();
void playPause();
void playStart();
void playPrevious();
void playNext();
void chooseMusic();

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

void setup() {
  Serial.begin(9600);
  delay(1500);
  
  for (int i = 0; i < quantLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  analogSetPinAttenuation(POT_PIN, ADC_11db);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  bool res;
  res = wm.autoConnect("DaltonEsp32","12345671");

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else { 
      Serial.println("Connected...yeey :)");
  }

  setupApi();

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  server.handleClient();

  int valPoten = analogRead(POT_PIN);
  valPoten = constrain(valPoten, 0, 4095);
  int ledNivel = map(valPoten, 0, 4095, 0, quantLeds);

  for (int i = 0; i < quantLeds; i++) {
    digitalWrite(ledPins[i], (i < ledNivel) ? HIGH : LOW);
  }
}

void setupApi() {
  server.on("/rebolado", playRebolado);
  server.on("/pause", playPause);
  server.on("/start", playStart);
  server.on("/previous", playPrevious);
  server.on("/next", playNext);
  server.on("/setMusic", HTTP_POST, chooseMusic);
 
  // start server
  server.begin();
}

void playRebolado(){
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Rick</title></head>"
                "<body style='text-align:center;'>"
                "<h1>Never Gonna Give You Up 🎵</h1>"
                "<img src='https://c.tenor.com/yheo1GGu3FwAAAAd/tenor.gif' alt='Rick Astley' style='max-width:90%;height:auto;'>"
                "</body></html>";
  
  server.send(200, "text/html", html);
}

void playPause() {
  Serial2.print("pause");
  server.send(200);
}

void playStart() {
  Serial2.print("start");
  server.send(200);
}

void playNext() {
  Serial2.print("next");
  server.send(200);
}

void playPrevious() {
  Serial2.print("previous");
  server.send(200);
}

void chooseMusic() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{msg: Error}");
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  
  int idMusic = jsonDocument["id"];
  Serial2.println("troca/" + String(idMusic));

  server.send(200, "application/json", "{}");
}