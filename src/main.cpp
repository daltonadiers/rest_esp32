#include <Arduino.h>

#define LED_QUARTO 22
#define LED_SALA 23
#define ADC_VREF_mV    3300.0 // in millivolt
#define ADC_RESOLUTION 4096.0
#define PIN_LM35 35 // ESP32 pin GPIO36 (ADC0) connected to LM35

void setup() {
  Serial.begin(9600);
  analogSetAttenuation(ADC_11db);

  pinMode(LED_QUARTO, OUTPUT);
  pinMode(LED_SALA, OUTPUT);

  digitalWrite(LED_QUARTO, 1);
  digitalWrite(LED_SALA, 1);
}

void loop() {
  // read the ADC value from the temperature sensor
  int adcVal = analogRead(PIN_LM35);
  // convert the ADC value to voltage in millivolt
  float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
  // convert the voltage to the temperature in °C
  float tempC = milliVolt / 10;
  // convert the °C to °F
  float tempF = tempC * 9 / 5 + 32;

  // print the temperature in the Serial Monitor:
  Serial.print("Temperature: ");
  Serial.print(tempC);   // print the temperature in °C
  Serial.print("°C");
  Serial.print("  ~  "); // separator between °C and °F
  Serial.print(tempF);   // print the temperature in °F
  Serial.println("°F");

  delay(500);
}