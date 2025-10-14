#include <Arduino.h>
#include <SPIFFS.h>
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
int ledPins[quantLeds] = {23, 21, 19, 18, 4, 2};
bool paused = true;

void setupApi();
void playRebolado();
void playPause();
void playStart();
void playPrevious();
void playNext();
void chooseMusic();
void getMusic();

void createJson(char *name, float value, char *unit)
{
    jsonDocument.clear();
    jsonDocument["name"] = name;
    jsonDocument["value"] = value;
    jsonDocument["unit"] = unit;
    serializeJson(jsonDocument, buffer);
}

void addJsonObject(char *name, int value)
{
    JsonObject obj = jsonDocument.createNestedObject();
    obj[name] = value;
}

void setup()
{
    Serial.begin(9600);
    delay(1500);

    for (int i = 0; i < quantLeds; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
    }

    analogSetPinAttenuation(POT_PIN, ADC_11db);

    WiFi.mode(WIFI_STA);
    WiFiManager wm;

    bool res;
    res = wm.autoConnect("DaltonEsp32", "12345671");

    if (!res)
    {
        Serial.println("Failed to connect");
        ESP.restart();
    }
    else
    {
        Serial.println("Connected...yeey :)");
    }

    setupApi();

    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop()
{
    server.handleClient();

    int valPoten = analogRead(POT_PIN);
    valPoten = constrain(valPoten, 0, 4095);
    int ledNivel = map(valPoten, 0, 4095, quantLeds, 0);

    for (int i = 0; i < quantLeds; i++)
    {
        digitalWrite(ledPins[i], (i < ledNivel) ? HIGH : LOW);
    }
}

void setupApi()
{
    server.on("/rebolado", playRebolado);
    server.on("/pause", playPause);
    server.on("/start", playStart);
    server.on("/previous", playPrevious);
    server.on("/next", playNext);
    server.on("/setMusic", HTTP_POST, chooseMusic);
    server.on("/getMusic", getMusic);

    // start server
    server.begin();
}

void playPause()
{
    Serial2.print("pause");
    server.send(200);
}

void playStart()
{
    Serial2.print("start");
    server.send(200);
}

void playNext()
{
    Serial2.print("next");
    server.send(200);
}

void playPrevious()
{
    Serial2.print("previous");
    server.send(200);
}

void chooseMusic()
{
    if (server.hasArg("plain") == false)
    {
        server.send(400, "application/json", "{msg: Error}");
    }
    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);

    int idMusic = jsonDocument["id"];
    Serial2.println("troca/" + String(idMusic));

    server.send(200, "application/json", "{}");
}

void getMusic()
{
    Serial2.println("getMusic");  // Envia comando para outra ESP

    unsigned long startTime = millis();
    const unsigned long timeout = 2000;

    while (millis() - startTime < timeout)
    {
        if (Serial2.available())
        {
            String msg = Serial2.readStringUntil('\n');
            int sepIndex = msg.indexOf('/');

            if (sepIndex >= 0)
            {
                int tocando = msg.substring(0, sepIndex).toInt();
                bool paused = msg.substring(sepIndex + 1).toInt();

                Serial.print("Tocando: ");
                Serial.println(tocando);
                Serial.print("Paused: ");
                Serial.println(paused);

                jsonDocument.clear();
                jsonDocument["id"] = tocando;
                jsonDocument["paused"] = paused;

                serializeJson(jsonDocument, buffer);
                server.send(200, "application/json", buffer);
                return;
            }
        }
    }
    Serial.println("Erro: resposta não recebida dentro do tempo limite");
    server.send(500, "text/plain", "Erro: sem resposta da outra ESP");
}

void playRebolado() {
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

  File file = SPIFFS.open("/rebolado.html");
  if(!file){
    server.send(404, "text/plain", "Arquivo não encontrado");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}