#include "audio-controller.h"
#include "display-ui.h"
#include "alarm-manager.h"

//* --- SD SOUND PLAYER ---
void playSDSound(const char* fileName) {
    if (!SD.exists(fileName)) {
        Serial.printf("SD Fájl nem található: %s\n", fileName);
        return;
    }

    AudioFileSourceSD *fileSD = new AudioFileSourceSD(fileName);
    
    // Helyi I2S kimenet létrehozása minden lejátszáshoz
    AudioOutputI2S *audioOut = new AudioOutputI2S();
    audioOut->SetPinout(34, 33, 13);
    audioOut->SetGain(0.35);

    AudioGeneratorMP3 *mp3 = new AudioGeneratorMP3();

    if (mp3->begin(fileSD, audioOut)) {
        Serial.printf("Lejátszás: %s\n", fileName);
        uint32_t lastCheck = 0;

        while (mp3->isRunning()) {
            if (!mp3->loop()) {
                mp3->stop();
            }

            if (millis() - lastCheck > 50) {
                lastCheck = millis();
                
                M5.update();
                auto touch = M5.Touch.getDetail(0);

                if (isRinging && (touch.wasPressed() || touch.isPressed())) {
                    isRinging = false;
                    mp3->stop();
                    break;
                }
            }
            vTaskDelay(1);
        }
    }

    delete mp3;
    delete fileSD;
    delete audioOut; // Tiszta lezárás: elengedi az I2S buszt a mikrofon számára
    Serial.println("SD hang lejátszás kész.");
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

    // 1. LÉPÉS: TTS letöltése az SD-re
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
            file.close();
        }
    } else {
        Serial.printf("[TTS HIBA] HTTP kód: %d\n", httpCode);
    }
    http.end();

    // 2. LÉPÉS: Lejátszás friss, helyi I2S kimenettel
    if (SD.exists("/sounds/tts.mp3")) {
        AudioFileSourceSD *fileSD = new AudioFileSourceSD("/sounds/tts.mp3");
        
        AudioOutputI2S *audioOut = new AudioOutputI2S();
        audioOut->SetPinout(34, 33, 13);
        audioOut->SetGain(0.35);

        AudioGeneratorMP3 *mp3Decoder = new AudioGeneratorMP3();

        if (mp3Decoder->begin(fileSD, audioOut)) {
            unsigned long lastDrawTime = 0;
            while (mp3Decoder->isRunning()) {
                if (!mp3Decoder->loop()) {
                    mp3Decoder->stop();
                }
                vTaskDelay(1);

                if (millis() - lastDrawTime > 50) {
                    lastDrawTime = millis();
                    M5.update();
                    drawUI();
                }
            }
        }

        delete mp3Decoder;
        delete fileSD;
        delete audioOut; // Tiszta törlés
    }

    isPlayingTTS = false;
    chatStateStartTime = millis();
}