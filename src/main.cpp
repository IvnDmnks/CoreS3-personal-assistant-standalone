#include "globals.h"
#include "display-ui.h"
#include "audio-controller.h"
#include "alarm-manager.h"
#include "ai-assistant.h"

//* --- SYNC REAL TIME ---
void syncTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) { 
        delay(500); 
        retry++;
    }
}

//* --- SETUP ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    Serial.begin(115200);

    // Hangerő és M5Speaker inicializálása szelíd szintre (0-255)
    M5.Speaker.begin();
    M5.Speaker.setVolume(120);

    canvas.createSprite(320, 240);
    rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.println("Inditas...");

    if(!SD.begin(SD_CS_PIN)) {
        Serial.println("Hiba: Az SD kartya nem indult el!");
    }

    wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
    wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);
    
    while(wifiMulti.run() != WL_CONNECTED) {
        delay(500);
    }

    syncTime();
    delay(1000);
}

//* --- LOOP ---
unsigned long lastAlarmCheck = 0;
unsigned long lastBeep = 0;

void loop() {
    M5.update();
    auto touch = M5.Touch.getDetail(0);

    // Ébresztő időpont ellenőrzése
    if(millis() - lastAlarmCheck >= 1000) {
        lastAlarmCheck = millis();
        checkAlarm();
    }

    // Asztalütögetés ellenőrzése az IMU-val
    checkTableKnock();

    // Ha éppen csörög az ébresztő:
    if(isRinging) {
    // 1. Kijelző kirajzolása PIROSRA az ébresztés pillanatában
    drawUI(); 

    // 2. Érintés ellenőrzése
    M5.update();
    auto touch = M5.Touch.getDetail(0);
    if (touch.wasPressed() || touch.isPressed()) {
        isRinging = false;
        M5.Speaker.stop();
        drawUI();
        playDailyBriefing();
        return;
    }

    // 3. Hang lejátszása
    playSDSound("/sounds/alarm.mp3");
    return;
}

    if(millis() - lastAlarmCheck >= 1000) {
        lastAlarmCheck = millis();
        checkAlarm();
    }

    if(currState == CHAT_STATE && !isRinging && !isPlayingTTS) {
        if (millis() - chatStateStartTime > 10000) {
            currState = CLOCK_STATE;
            lastUserSpeech = "";
            lastAIReply = "";

            clearChatHistory();
        }
    }

    if (touch.wasPressed()) {
        if(touch.y < 120) playDailyBriefing(); //* upper side of the screen was pressed -> daily briefing 
        else {  //* bottom side of the screen was pressed -> STT
            startRecordingUI();
            drawUI();
            int i = 0;
            while(M5.Touch.getDetail(0).isPressed() && i < record_size) {
                M5.update();
                if(M5.Mic.record(&rec_data[i], record_length, record_samplerate)) i += record_length;
            }
            stopRecordingUI();
            drawUI();

            uint32_t recorded_bytes = i * 2;
            byte wavHeader[44];
            createWavHeader(wavHeader, recorded_bytes);

            String whisperResponse = sendAudioToWhisper(wavHeader, recorded_bytes);

            Serial.println("[WHISPER NYERS VÁLASZ]:");
            Serial.println(whisperResponse);
            
            DynamicJsonDocument whisperDoc(1024);
            deserializeJson(whisperDoc, whisperResponse);
            String transcribedText = whisperDoc["text"].as<String>();

            int16_t maxVolume = 0;
            for (int j = 0; j < i; j++) {
                if (abs(rec_data[j]) > maxVolume) maxVolume = abs(rec_data[j]);
            }

            Serial.println("====================================");
            Serial.printf("[MIC TEST] Max felvett hangerő: %d\n", maxVolume);
            Serial.printf("[WHISPER] Felismert szöveg: '%s'\n", transcribedText.c_str());
            Serial.println("====================================");

            lastUserSpeech = transcribedText;
            
            if (transcribedText.length() > 0) {
                if (!processIntent(transcribedText)) {
                    playSDSound(SOUND_ERROR);
                    M5.Display.fillScreen(BLACK);
                    M5.Display.setCursor(10, 50);
                    playTTS("Sajnos nem ertettem, amit mondtal.");
                }
            }

        }
    }

    drawUI();
}