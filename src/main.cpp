#include <M5CoreS3.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "mbedtls/base64.h"

// Titkos adatok behúzása (a mi .env megfelelőnk)
#include "secrets.h"

#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceHTTPStream.h"

const char* CITY = "Budapest";
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 3600;
const int   DAYLIGHT_OFFSET_SEC = 3600;

AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

// --- IDŐ SZINKRONIZÁLÁSA ---
void syncTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) { delay(500); }
}

// --- APPLE BASE64 HITELESÍTÉS ---
String getAppleAuthHeader() {
    String auth = String(APPLE_ID) + ":" + String(APPLE_APP_PASSWORD);
    unsigned char base64_buf[128];
    size_t olen = 0;
    mbedtls_base64_encode(base64_buf, 128, &olen, (const unsigned char*)auth.c_str(), auth.length());
    return "Basic " + String((char*)base64_buf);
}

// --- IDŐJÁRÁS LEKÉRÉSE ---
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

// --- TÖBB NAPTÁR BÖNGÉSZÉSE ---
String get_today_events() {
    String all_events = "";
    WiFiClientSecure client;
    client.setInsecure(); // SSL csekkolás kihagyása
    HTTPClient http;
    
    String xmlRequest = 
        "<c:calendar-query xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">"
        "  <d:prop><c:calendar-data/></d:prop>"
        "  <c:filter><c:comp-filter name=\"VCALENDAR\"><c:comp-filter name=\"VEVENT\"/></c:comp-filter></c:filter>"
        "</c:calendar-query>";

    // Végigmegyünk az összes naptáron, amit a secrets.h-ban megadtál
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

// --- GPT ÖSSZEFOGLALÓ ---
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

// --- TTS LEJÁTSZÁS (ELŐKÉSZÍTVE) ---
void playTTS(String text) {
    CoreS3.Display.clear();
    CoreS3.Display.setCursor(0, 5);
    CoreS3.Display.println("Hangszintetizalas...");

    out = new AudioOutputI2S();
    out->SetPinout(34, 33, 0); 
    file = new AudioFileSourceHTTPStream();
    
    CoreS3.Display.println("Stream (teszt) indul...");
}

// --- FŐ LOGIKA (Reggeli Rutin) ---
void playDailyBriefing() {
    CoreS3.Display.clear();
    CoreS3.Display.setCursor(0, 5);
    CoreS3.Display.println("Adatok letoltese...");

    String weather_info = get_weather();
    String events_str = get_today_events(); // Már az ÖSSZES naptárat nézi
    
    String prompt_context = "Mai idojaras: " + weather_info + "\n" +
                            "Mai naptarbejegyzesek: " + events_str;
                            
    CoreS3.Display.println("AI general...");
    String system_prompt = "Egy asztali AI asszisztens vagy. Foglald ossze a napot kozvetlen, baratsagos hangnemben, magyarul! Maximum 3-4 mondat.";
    
    String ai_text = getGPTSummary(system_prompt, prompt_context);
    
    CoreS3.Display.clear();
    CoreS3.Display.setCursor(0, 0);
    CoreS3.Display.println(ai_text);
    
    playTTS(ai_text);
}

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.println("Inditas...");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        CoreS3.Display.print(".");
    }
    
    syncTime(); 

    CoreS3.Display.clear();
    CoreS3.Display.setCursor(0, 10);
    CoreS3.Display.println("Kesz! Erintsd meg!");
}

void loop() {
    CoreS3.update();

    if (CoreS3.Touch.getCount() > 0) {
        auto touch = CoreS3.Touch.getDetail(0);
        if (touch.wasPressed()) {
            playDailyBriefing();
            delay(2000); 
        }
    }
}