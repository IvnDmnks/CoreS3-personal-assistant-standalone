#include "audio-controller.h"
#include "display-ui.h"
#include "alarm-manager.h"

//* --- SD SOUND PLAYER ---
void playSDSound(const char* fileName) {
    if(!SD.exists(fileName)) {
        Serial.printf("Hiba: Nem talalhato hangfajl: %s\n", fileName);
        return;
    }

    AudioFileSourceSD *fileSD = new AudioFileSourceSD(fileName);
    AudioOutputI2S *audioOutSD = new AudioOutputI2S();
    audioOutSD->SetPinout(34, 33, 13);

    AudioGeneratorMP3 *mp3Decoder = new AudioGeneratorMP3();
    mp3Decoder->begin(fileSD, audioOutSD);

    while(mp3Decoder->isRunning()) {
        if(!mp3Decoder->loop()) {
            mp3Decoder->stop();
        }
        M5.update();
        auto touch = M5.Touch.getDetail(0);
        
        if (isRinging && (touch.wasPressed() || touch.isPressed())) {
            mp3Decoder->stop();
            isRinging = false;
            break;
        }
    }

    delete mp3Decoder;
    delete audioOutSD;
    delete fileSD;
}

void playTTS(String text) {
    if(text.length() == 0) return;
    isPlayingTTS = true;

    HTTPClient http;
    http.begin("https://api.openai.com/v1/audio/speech");
    http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(30000);

    DynamicJsonDocument doc(2048);
    doc["model"] = "tts-1";
    doc["input"] = text;
    doc["voice"] = "nova";
    doc["response_format"] = "mp3";
    doc["speed"] = 1.15;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);

    // 1. LÉPÉS: A teljes MP3 fájl letöltése és lezárása az SD kártyán
    if (httpCode == HTTP_CODE_OK) {
        File file = SD.open("/sounds/tts.mp3", FILE_WRITE);
        if (file) {
            WiFiClient *stream = http.getStreamPtr();
            uint8_t buff[1024];
            unsigned long lastDataTime = millis();

            while (http.connected() || stream->available()) {
                size_t size = stream->available();
                if (size > 0) {
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    file.write(buff, c);
                    lastDataTime = millis();
                } else {
                    delay(1);
                    if (millis() - lastDataTime > 5000) break; 
                }
            }
            file.close(); // FÁJL LEZÁRÁSA: Így az egész hangfájl hiánytalanul az SD-re kerül!
        }
    } else {
        Serial.printf("[TTS HIBA] HTTP kód: %d\n", httpCode);
    }
    http.end();

    // 2. LÉPÉS: Lejátszás az SD kártyáról az legutolsó másodpercig
    if (SD.exists("/sounds/tts.mp3")) {
        AudioFileSourceSD *fileSD = new AudioFileSourceSD("/sounds/tts.mp3");
        AudioOutputI2S *audioOutSD = new AudioOutputI2S();
        
        audioOutSD->SetPinout(34, 33, 13); 
        audioOutSD->SetGain(0.35); // Recsegésmentes, tiszta hangerő

        AudioGeneratorMP3 *mp3Decoder = new AudioGeneratorMP3();
        mp3Decoder->begin(fileSD, audioOutSD);

        unsigned long lastDrawTime = 0;
        while (mp3Decoder->isRunning()) {
            if (!mp3Decoder->loop()) {
                mp3Decoder->stop();
            }
            delay(1);
            if (millis() - lastDrawTime > 40) {
                lastDrawTime = millis();
                M5.update();
                drawUI();
            }
        }

        delete mp3Decoder;
        delete audioOutSD;
        delete fileSD;
    }

    isPlayingTTS = false;
    chatStateStartTime = millis();
}