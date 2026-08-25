#include <M5CoreS3.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "mbedtls/base64.h"

#include "secrets.h"

#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceHTTPStream.h"
#include <ArduinoJson.h>

//* ---------------
//* --- GLOBALS ---
//* ---------------

const char* CITY = "Budapest";
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 3600;
const int   DAYLIGHT_OFFSET_SEC = 3600;

// ------------------------------------------------------------------

WiFiMulti wifiMulti;

// ------------------------------------------------------------------

static constexpr const size_t record_number = 512;
static constexpr const size_t record_length = 320;
static constexpr const size_t record_size = record_number * record_length;
static constexpr const size_t record_samplerate = 16000; //* Whisper API - 16000 Hz sampling
static int16_t *rec_data = nullptr;
static size_t rec_record_idx  = 2;
/*
    512 * 320 = 163840 sample
    163840/16000 = 10,24 sec record time
    163840 * 2 = 327,68 KB / record 
*/

// ------------------------------------------------------------------

bool isAlarmSet = false;
int alarmHour = 0;
int alarmMinute = 0;
bool isRinging = false;
bool isPlayingTTS = false;
bool isRecording = false;

// ------------------------------------------------------------------

enum UIState {
    CLOCK_STATE,
    CHAT_STATE
};

UIState currState = CLOCK_STATE;

M5Canvas canvas(&M5.Display);

String lastUserSpeech = "";
String lastAIReply = "";
String statusMessage = "";

unsigned long chatStateStartTime = 0;

// ------------------------------------------------------------------
// ------------------------------------------------------------------

AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

//* --- SYNC REAL TIME ---
void syncTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) { delay(500); }
}

// ------------------------------------------------------------------

//* --- APPLE BASE64 AUTH ---
String getAppleAuthHeader() {
    String auth = String(APPLE_ID) + ":" + String(APPLE_APP_PASSWORD);
    unsigned char base64_buf[128];
    size_t olen = 0;
    mbedtls_base64_encode(base64_buf, 128, &olen, (const unsigned char*)auth.c_str(), auth.length());
    return "Basic " + String((char*)base64_buf);
}

// ------------------------------------------------------------------

//* --- WEATHER API ---
String get_weather() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(CITY) + "&appid=" + String(OPENWEATHER_API_KEY) + "&units=metric&lang=hu";
    http.begin(url);
    int httpCode = http.GET();
    String result = "Idojaras hiba.";

    if (httpCode == HTTP_CODE_OK) {
        StaticJsonDocument<1024> doc;
        deserializeJson(doc, http.getString());
        float temp = doc["main"]["temp"];
        const char* desc = doc["weather"][0]["description"];
        result = String(CITY) + ": " + String(desc) + ", " + String(temp, 1) + " fok.";
    }
    http.end();
    return result;
}

// ------------------------------------------------------------------

//* --- CALENDARS CHECKING ---
String get_today_events() {
    String all_events = "";
    WiFiClientSecure client;
    client.setInsecure(); //* SSL check skip, less ram usage
    HTTPClient http;
    
    String xmlRequest = 
        "<c:calendar-query xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">"
        "  <d:prop><c:calendar-data/></d:prop>"
        "  <c:filter><c:comp-filter name=\"VCALENDAR\"><c:comp-filter name=\"VEVENT\"/></c:comp-filter></c:filter>"
        "</c:calendar-query>";

    for (int i = 0; i < NUM_CALENDARS; i++) {
        http.begin(client, CALENDAR_URLS[i]);
        http.addHeader("Authorization", getAppleAuthHeader());
        http.addHeader("Content-Type", "application/xml; charset=utf-8");
        http.addHeader("Depth", "1");

        int httpCode = http.sendRequest("REPORT", xmlRequest);
        
        if (httpCode == 200 || httpCode == 207) {
            String response = http.getString();
            int index = 0;
            while ((index = response.indexOf("SUMMARY:", index)) != -1) {
                int endIndex = response.indexOf("\n", index);
                if (endIndex != -1) {
                    String eventName = response.substring(index + 8, endIndex);
                    eventName.trim();
                    all_events += eventName + ", ";
                    index = endIndex;
                } else break;
            }
        }
        http.end();
    }
    
    if (all_events == "") return "Ma nincs esemeny a naptaraidban.";
    return all_events;
}

// ------------------------------------------------------------------

//* --- GPT SUMMARY ---
String getGPTSummary(String systemRole, String userContext) {
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;
    
    http.begin(client, "https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));

    StaticJsonDocument<2048> doc;
    doc["model"] = "gpt-4o-mini";
    doc["max_tokens"] = 150;
    
    JsonArray messages = doc.createNestedArray("messages");
    
    JsonObject msgSystem = messages.createNestedObject();
    msgSystem["role"] = "system";
    msgSystem["content"] = systemRole;
    
    JsonObject msgUser = messages.createNestedObject();
    msgUser["role"] = "user";
    msgUser["content"] = userContext;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    String responseText = "AI hiba.";

    if (httpCode == HTTP_CODE_OK) {
        StaticJsonDocument<2048> responseDoc;
        deserializeJson(responseDoc, http.getString());
        responseText = responseDoc["choices"][0]["message"]["content"].as<String>();
    }
    http.end();
    return responseText;
}

// ------------------------------------------------------------------

//* --- TEXT TO SPEECH ---
class AudioFileSourceWiFiClient : public AudioFileSource {
    private: 
        WiFiClientSecure *client;
    public:
        AudioFileSourceWiFiClient(WiFiClientSecure *c) : client(c) {}
        virtual bool open(const char* filename) override { return true; }
        virtual uint32_t read(void *data, uint32_t len) override {
            if(!client) return 0;
            return client->read((uint8_t*)data, len);
        }
        virtual bool seek(int32_t offset, int direction) override { return false; }
        virtual bool close() override { return true; }
        virtual bool isOpen() override { return client && (client->connected() || client->available()); }
        virtual uint32_t getSize() override { return 0; }
        virtual uint32_t getPos() override { return 0; }
};

void playTTS(String text) {
    if(text.length() == 0) return;
    isPlayingTTS = true;

    WiFiClientSecure client;
    client.setInsecure();

    if(!client.connect("api.openai.com", 443)) {
        isPlayingTTS = false;
        return;
    }

    StaticJsonDocument<512> doc; 
    doc["model"] = "tts-1";
    doc["input"] = text;
    doc["voice"] = "nova"; //* Optional voices: alloy, echo, fable, onyx, nova, shimmer
    doc["response_format"] = "mp3";

    String requestBody;
    serializeJson(doc, requestBody);

    client.println("POST /v1/audio/speech HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " + String(OPENAI_API_KEY));
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(requestBody.length());
    client.println("Connection: close");
    client.println();
    client.print(requestBody);

    while(client.connected()) {
        String line = client.readStringUntil('\n');
        if(line == "\r") break;
    }

    AudioFileSourceWiFiClient *fileSource = new AudioFileSourceWiFiClient(&client);
    AudioOutputI2S *audioOut = new AudioOutputI2S();

    audioOut->SetPinout(34, 33, 0);

    AudioGeneratorMP3 *mp3Decoder = new AudioGeneratorMP3();
    mp3Decoder->begin(fileSource, audioOut);

    while(mp3Decoder->isRunning()) {
        if(!mp3Decoder->loop()) {
            mp3Decoder->stop();
        }
        M5.update();
    }

    delete mp3Decoder;
    delete audioOut;
    delete fileSource;
    client.stop();

    isPlayingTTS = false;
    chatStateStartTime = millis();
}

// ------------------------------------------------------------------

//* --- GTP INTENT ---
bool processIntent(String text) {
    HTTPClient http;
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
    
    DynamicJsonDocument doc(1024);
    doc["model"] = "gpt-4o-mini";
    JsonObject response_format = doc.createNestedObject("response_format");
    response_format["type"] = "json_object";
    JsonArray messages = doc.createNestedArray("messages");
    JsonObject systemMsg = messages.createNestedObject();
    systemMsg["role"] = "system";
    systemMsg["content"] = "Te egy okosóra AI asszisztense vagy. A felhasználó mondata alapján dötsd el a szándékot! "
                           "KIZÁRÓLAG egy érvényes JSON-t adj vissza! "
                           "1. Ébresztésnél: {\"type\": \"alarm\", \"hour\": 7, \"minute\": 30} "
                           "2. Kérdésnél/beszélgetésnél válaszolj röviden, max 2 mondatban: "
                           "{\"type\": \"chat\", \"reply\": \"A válaszod...\"}";
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = text;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    statusMessage = "Gondolkodom...";
    int httpResponseCode = http.POST(requestBody);
    
    if(httpResponseCode == 200) {
        String response = http.getString();
        DynamicJsonDocument responseDoc(2048);
        deserializeJson(responseDoc, response);
        String gptJsonString = responseDoc["choices"][0]["message"]["content"].as<String>();
        
        DynamicJsonDocument resultDoc(1024);
        deserializeJson(resultDoc, gptJsonString);
        
        String intentType = resultDoc["type"].as<String>();
        
        if(intentType == "alarm") {
            alarmHour = resultDoc["hour"];
            alarmMinute = resultDoc["minute"];
            isAlarmSet = true;
            char buff[64];
            sprintf(buff, "Ebreszto beallitva %02d:%02d-ra.", alarmHour, alarmMinute);
            statusMessage = "Ebreszto beallitva!";
            lastAIReply = String(buff);
            playTTS(String(buff));
            http.end();
            return true;
        } 
        else if(intentType == "chat") {
            String aiReply = resultDoc["reply"].as<String>();
            statusMessage = "Valaszolok...";
            lastAIReply = aiReply;
            playTTS(aiReply);
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}

// ------------------------------------------------------------------

//* --- SPEECH TO TEXT | PUSH TO TALK ---
void startRecordingUI() {
    isRecording = true;
    M5.Mic.begin();
    currState = CHAT_STATE;
    statusMessage = "Hallgatlak...";
    lastUserSpeech = "";
    lastAIReply = "";
    memset(rec_data, 0, record_size * sizeof(int16_t));
}

void stopRecordingUI() {
    isRecording = false;
    M5.Mic.end();
    statusMessage = "Feldolgozas...";
}

void createWavHeader(byte* header, uint32_t waveDataSize) {
    uint32_t sampleRate = 16000;
    uint16_t numChannels = 1;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    uint32_t fileSize = 36 + waveDataSize;

    //* "RIFF" chunk descriptor
    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    header[4] = (byte)(fileSize & 0xFF);
    header[5] = (byte)((fileSize >> 8) & 0xFF);
    header[6] = (byte)((fileSize >> 16) & 0xFF);
    header[7] = (byte)((fileSize >> 24) & 0xFF);
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
    
    //* "fmt" sub-chunk
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; //* Subchunk1Size (16 for PCM)
    header[20] = 1; header[21] = 0; //* AudioFormat (1 for PCM)
    header[22] = (byte)numChannels; header[23] = 0;
    header[24] = (byte)(sampleRate & 0xFF);
    header[25] = (byte)((sampleRate >> 8) & 0xFF);
    header[26] = (byte)((sampleRate >> 16) & 0xFF);
    header[27] = (byte)((sampleRate >> 24) & 0xFF);
    header[28] = (byte)(byteRate & 0xFF);
    header[29] = (byte)((byteRate >> 8) & 0xFF);
    header[30] = (byte)((byteRate >> 16) & 0xFF);
    header[31] = (byte)((byteRate >> 24) & 0xFF);
    header[32] = (byte)(numChannels * bitsPerSample / 8); header[33] = 0; //* BlockAlign
    header[34] = (byte)bitsPerSample; header[35] = 0;
    
    //* "data" sub-chunk
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    header[40] = (byte)(waveDataSize & 0xFF);
    header[41] = (byte)((waveDataSize >> 8) & 0xFF);
    header[42] = (byte)((waveDataSize >> 16) & 0xFF);
    header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}

String sendAudioToWhisper(byte* wavHeader, uint32_t recorded_bytes) {
    WiFiClientSecure client;
    client.setInsecure(); //* SSL check skip, less ram usage

    M5.Display.drawString("Csatlakozas...", 180, 3);

    if(!client.connect("api.openai.com", 443)) return "Hiba: Nem siikerult csatlakozni";

    M5.Display.drawString("Feltoltes...", 180, 3);

    String boundary = "----Esp32Boundary12345";

    //* start of multipart message
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    head += "whisper-1\r\n";
    head += "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    head += "Content-Type: audio/wav\r\n\r\n";

    //* end of multipart message
    String tail = "\r\n" + boundary + "--\r\n";

    uint32_t contentLength = head.length() + 44 + recorded_bytes + tail.length();

    //* HTTP POST header
    client.println("POST /v1/audio/transcriptions HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " + String(OPENAI_API_KEY)); 
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();

    //* HTTP POST body
    client.print(head);
    client.write(wavHeader, 44);

    uint8_t* audioData = (uint8_t*) rec_data;
    size_t bytesRemaining = recorded_bytes;
    size_t offset = 0; 
    size_t chunkSize = 1024; //* sending audio data in 1024-byte chunks

    while(bytesRemaining > 0) {
        size_t bytesToWrite = (bytesRemaining < chunkSize) ? bytesRemaining : chunkSize;
        client.write(audioData + offset, bytesToWrite);
        offset += bytesToWrite;
        bytesRemaining -= bytesToWrite;
    }

    //* multipart termination
    client.print(tail);

    //* Read response
    String response = "";
    bool headerPassed = false;
    while(client.connected()) {
        String line = client.readStringUntil('\n');
        if(line == "\r") {
            headerPassed = true;
            break;
        } 
    }

    if(headerPassed) response = client.readString();

    client.stop();
    return response;
}

// ------------------------------------------------------------------

//* --- MORNING ROUTINE ---
void playDailyBriefing() {
    currState = CHAT_STATE;
    statusMessage = "Napi osszefoglalo...";
    lastUserSpeech = "Napi osszefoglalo keres";
    lastAIReply = "Adatok letoltese...";

    String weather_info = get_weather();
    String events_str = get_today_events();
    
    String prompt_context = "Mai idojaras: " + weather_info + "\n" + "Mai naptarbejegyzesek: " + events_str;
    String system_prompt = "Egy asztali AI asszisztens vagy. Foglald ossze a napot kozvetlen, baratsagos hangnemben, magyarul! Maximum 3-4 mondat.";
                            
    lastAIReply = "AI general...";
    
    String ai_text = getGPTSummary(system_prompt, prompt_context);
    
    lastAIReply = ai_text;
    
    playTTS(ai_text);
}

// ------------------------------------------------------------------

//* --- ALARM CLOCK ---
void checkAlarm() {
    if(!isAlarmSet || isRinging) return;
    
    struct tm timeInfo;
    if(getLocalTime(&timeInfo)) {
        if(timeInfo.tm_hour == alarmHour && timeInfo.tm_min == alarmMinute) {
            isAlarmSet = false; //* alarm set off, otherwise morning routine would run 60 times per minute
            isRinging = true;
        }
    }
}

void checkTableKnock() {
    float ax, ay, az;
    M5.Imu.getAccelData(&ax, &ay, &az);
    float totalAccelerate = sqrt(ax*ax + ay*ay + az*az);
    if(totalAccelerate > 2.0) {
        M5.Speaker.stop();
        isRinging = false;
        playDailyBriefing();
    }
}

// ------------------------------------------------------------------

//* --- SETUP ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    canvas.createSprite(320, 240);

    rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.println("Inditas...");
    
    syncTime(); 

    M5.Display.println("WiFi halozatok beallitasa...");
    wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
    wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);

    M5.Display.print("Csatlakozas...");

    while(wifiMulti.run() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
    }

    M5.Display.println("");
    M5.Display.print("Sikeresen csatlakozva ehhez: ");
    M5.Display.println(WiFi.SSID());
    M5.Display.print("IP cim: ");
    M5.Display.println(WiFi.localIP());
}

// ------------------------------------------------------------------

//* --- UI ---
void drawUI() {
    canvas.fillScreen(BLACK);

    if(currState == CLOCK_STATE) {
        //* --- DIGITAL CLOCK ---
        //* top bar
        canvas.setTextDatum(TC_DATUM); //* top-center
        canvas.setTextColor(DARKGREY);
        canvas.setTextSize(1);
        if(isAlarmSet) {
            char alarmBuffer[32];
            sprintf(alarmBuffer, "Ebreszto: %02d:%02d", alarmHour, alarmMinute);
            canvas.drawString(alarmBuffer, 160, 10);
        } else {
            canvas.drawString("Ebreszto: Nincs beallitva", 160, 10);
        }

        //* middle bar
        struct tm timeinfo;
        if(getLocalTime((&timeinfo))) {
            char timeBuffer[10];
            sprintf(timeBuffer, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

            canvas.setTextColor(CYAN, BLACK);
            canvas.setTextDatum(MC_DATUM); //* middle-center
            canvas.setTextSize(5);
            canvas.drawString(timeBuffer, 160, 95);

            char dateBuffer[32];
            sprintf(dateBuffer, "%04d.%02d.%02d.", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
            canvas.setTextColor(WHITE);
            canvas.setTextSize(2);
            canvas.drawString(dateBuffer, 160, 140);
        } else {
            canvas.setTextColor(RED);
            canvas.setTextDatum(MC_DATUM);
            canvas.setTextSize(2);
            canvas.drawString("Ido szinkroniizalasa...", 160, 95);
        }

        //* bottom bar
        canvas.drawRoundRect(10, 185, 300, 45, 8, GREEN);
        canvas.setTextColor(GREEN);
        canvas.setTextSize(2);
        canvas.drawString("TARTSD NYOMVA A BESZEDHEZ", 160, 207);
    }
    else if(currState == CHAT_STATE) {
        //* --- CHAT BOT VIEW ---
        //* top bar
        canvas.fillRect(0, 0, 320,  30, NAVY);
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.drawString(statusMessage, 160, 15);

        //* text box
        canvas.setTextDatum(TL_DATUM); //* top-left
        canvas.setTextSize(2);
        canvas.setTextWrap(true);

        //* user side
        if(lastUserSpeech.length() > 0) {
            canvas.setTextColor(GREEN);
            canvas.drawString("Te: ", 10, 40);
            canvas.setTextColor(WHITE);
            canvas.setCursor(10, 60);
            canvas.print(lastUserSpeech);
        }

        //* ai reply
        if(lastAIReply.length() > 0) {
            canvas.setTextColor(CYAN);
            canvas.drawString("Ai: ", 10, 125);
            canvas.setTextColor(WHITE);
            canvas.setCursor(10, 145);
            canvas.print(lastAIReply);
        }
    }

    if (isRinging) {
        canvas.fillScreen(RED);
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(4);
        canvas.drawString("EBRESZTO!", 160, 100);
        canvas.setTextSize(2);
        canvas.drawString("Erints/Uss a szundihoz!", 160, 160);
    }

    canvas.pushSprite(0, 0);
}

// ------------------------------------------------------------------

//* --- LOOP ---
unsigned long lastAlarmCheck = 0;
unsigned long lastBeep = 0;

void loop() {
    M5.update();
    auto touch = M5.Touch.getDetail(0);

    if (wifiMulti.run() != WL_CONNECTED) {
        Serial.println("WiFi kapcsolat megszakadt, ujracsatlakozas...");
        delay(1000); 
    }

    if(isRinging) {
        checkTableKnock();
        if(millis() - lastBeep > 500) {
            lastBeep = millis();
            M5.Speaker.tone(1000, 200);
        }
        if(touch.wasPressed()) {
            M5.Speaker.stop();
            isRinging = false;
            playDailyBriefing();
        }
        drawUI();
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
        }
    }

    if (touch.wasPressed()) {
        if(touch.y < 120) playDailyBriefing(); //* upper side of the screen was pressed -> daily briefing 
        else {  //* bottom side of the screen was pressed -> STT
            startRecordingUI();
            int i = 0;
            while(M5.Touch.getDetail(0).isPressed() && i < record_size) {
                M5.update();
                drawUI();
                if(M5.Mic.record(&rec_data[i], record_length, record_samplerate)) i += record_length;
            }
            stopRecordingUI();
            drawUI();

            uint32_t recorded_bytes = i * 2;
            byte wavHeader[44];
            createWavHeader(wavHeader, recorded_bytes);

            String whisperResponse = sendAudioToWhisper(wavHeader, recorded_bytes);
            
            DynamicJsonDocument whisperDoc(1024);
            deserializeJson(whisperDoc, whisperResponse);
            String transcribedText = whisperDoc["text"].as<String>();
            
            if (transcribedText.length() > 0) {
                if (!processIntent(transcribedText)) {
                    M5.Display.fillScreen(BLACK);
                    M5.Display.setCursor(10, 50);
                    playTTS("Sajnos nem ertettem, amit mondtal.");
                }
            }

        }
    }

    drawUI();
}