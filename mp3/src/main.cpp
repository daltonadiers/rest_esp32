#include <SPI.h>
#include <SD.h>
#include "BluetoothA2DPSource.h"

#define RXD2 16
#define TXD2 17
#define POT_PIN 34
#define VOL_MIN 0
#define VOL_MAX 127

BluetoothA2DPSource a2dp;
File f;
const int CS = 5;
const int NUM_MUSICAS = 100;
int tocando = 1;

volatile bool paused = true;

bool open_wav_data()
{
  String wav = "/musics/" + String(tocando) + ".wav";
  Serial.println(wav);
  
  f = SD.open(wav, FILE_READ);
  if (!f) {
    Serial.println("False");
    return false;
  }
  f.seek(44); // cabeçalho WAV padrão
  return true;
}

// callback em "frames" (L,R 16-bit => 4 bytes por frame)
int32_t get_data(Frame *frame, int32_t frame_count)
{
  if (!f)
    return 0;

  if (paused)
  {
    // envia silêncio mantendo o stream ativo
    memset(frame, 0, frame_count * sizeof(Frame));
    return frame_count;
  }

  int32_t want = frame_count * 4;
  int readb = f.read((uint8_t *)frame, want);
  if (readb <= 0)
    return 0;
  return readb / 4;
}

int lastVol = -1;
float filt = 0;
const float ALPHA = 0.2f;
unsigned long lastVolMs = 0;

int adc_to_volume(int adc) {
  float x = (4095 - adc) / 4095.0f;  // Inverte o valor do ADC
  float y = powf(x, 0.6f);
  int v = (int)roundf(VOL_MIN + y * (VOL_MAX - VOL_MIN));
  if (v < VOL_MIN) v = VOL_MIN;
  if (v > VOL_MAX) v = VOL_MAX;
  return v;
}

void setup()
{
  Serial.begin(115200);
  SD.begin(CS);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(POT_PIN, INPUT);

  a2dp.set_data_callback_in_frames(get_data);
  a2dp.set_volume(90);
  a2dp.start("Tronsmart Trip");

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Comandos: '1' toca, 'pause' pausa, 'start' continua.");
}

void loop()
{
  if (Serial2.available())
  {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    if (cmd == "start")
    {
      paused = false;
      if (!f)
        open_wav_data();
      Serial.println("Continua a tocar.");
    }
    else if (cmd == "pause")
    {
      paused = true;
      Serial.println("Pausado (silêncio enviado no A2DP).");
    }
    else if (cmd == "previous")
    {
      paused = false;
      tocando--;
      if (tocando < 1)
        tocando = NUM_MUSICAS;

      if (f)
        f.close();
      open_wav_data();
    }
    else if (cmd == "next")
    {
      paused = false;
      tocando++;
      if (tocando > NUM_MUSICAS)
        tocando = 1;

      if (f)
        f.close();
      open_wav_data();
    }
    else if (cmd.startsWith("troca/")) {
      String idStr = cmd.substring(6);
      
      int id = idStr.toInt();
      Serial.println("Id " + String(id));
      if (id <= NUM_MUSICAS && id >= 1 && id != tocando) {
        tocando = id;

        if(f)
          f.close();

        paused = false;
        open_wav_data();
      } 
    }
    else if (cmd == "getMusic") {
      unsigned long pos = 0;
      if(f) pos = (f.position() - 44) / 176400;
      char msg[20];
      snprintf(msg, sizeof(msg), "%d/%d/%lu", tocando, paused, pos);
      Serial2.println(msg);
    }
  }

  // fim do arquivo
  if (f && !f.available() && !paused)
  {
    f.close();
    Serial.println("Fim da musica. Passando para a próxima.");
    tocando++;
    if(tocando > NUM_MUSICAS)
      tocando = 1;
    open_wav_data();
  }

  unsigned long now = millis();
  if (now - lastVolMs >= 100) {
    lastVolMs = now;
    int raw = analogRead(POT_PIN);
    filt = (lastVol < 0) ? raw : (ALPHA*raw + (1-ALPHA)*filt);
    int vol = adc_to_volume((int)filt);
    if (lastVol < 0 || abs(vol - lastVol) >= 2) {
      a2dp.set_volume(vol);
      lastVol = vol;
    }
  }
}
