#include "ai-assistant.h"
#include "audio-controller.h"
#include "globals.h"

//* --- APPLE BASE64 AUTH ---
String getAppleAuthHeader() {
    String auth = String(APPLE_ID) + ":" + String(APPLE_APP_PASSWORD);
    unsigned char base64_buf[128];
    size_t olen = 0;
    mbedtls_base64_encode(base64_buf, 128, &olen, (const unsigned char*)auth.c_str(), auth.length());
    return "Basic " + String((char*)base64_buf);
}

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
    doc["max_tokens"] = 60;
    
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

//* --- GTP INTENT ---
void clearChatHistory() {
    if (SD.exists(CHAT_HISTORY_FILE)) {
        SD.remove(CHAT_HISTORY_FILE);
        Serial.println("[CHAT] Előzmények törölve az SD-ről.");
    }
}

bool processIntent(String text) {
    HTTPClient http;
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
    http.setTimeout(15000);

    DynamicJsonDocument doc(4096);
    doc["model"] = "gpt-4o-mini";
    
    // 1. SD-ről korábbi előzmények betöltése
    if (SD.exists(CHAT_HISTORY_FILE)) {
        File f = SD.open(CHAT_HISTORY_FILE, FILE_READ);
        deserializeJson(doc, f);
        f.close();
    } else {
        // Ha még nincs előzmény, beállítjuk az alap rendszert
        JsonObject response_format = doc.createNestedObject("response_format");
        response_format["type"] = "json_object";
        
        JsonArray messages = doc.createNestedArray("messages");
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = R"(
    Te egy M5CoreS3 okosóra intelligens, barátságos és lényegretörő AI asszisztense vagy. 

    KIZÁRÓLAG egy érvényes JSON objektumot adj vissza, semmi más szöveget!

    SZABÁLYOK:
    1. ÉBRESZTŐ BEÁLLÍTÁSA:
    Ha a felhasználó ébresztőt akar beállítani (pl. "Ébressz fel 7:30-kor", "Állíts ébresztőt nyolckor"):
    {"type": "alarm", "hour": 7, "minute": 30}

    2. ÁLTALÁNOS CHAT ÉS KÉRDÉSEK:
    Minden más esetben (köszönések, tudományos/általános kérdések, viccek, tanácsok, fordítás, matek):
    {"type": "chat", "reply": "A válaszod..."}

    KÖVETELMÉNYEK A CHAT VÁLASZOKHOZ (reply):
    - Maximum 1-3 rövid, tömör mondatban válaszolj!
    - A válaszod legyen természetes és jól érthető felolvasva (TTS hangszóróhoz optimalizálva).
    - Ha valaminek a köszöntését kérik (pl. "Köszönj Annának"), válaszolj közvetlenül (pl. "Szia Anna! Szép napot kívánok!").
    - Általános tudásalapú kérdésekre adj pontos, közvetlen választ mellébeszélés nélkül.
    )" ;
    }

    // 2. Felhasználó új mondatának hozzáadása a beszélgetéshez
    JsonArray messages = doc["messages"];
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = text;

    String requestBody;
    serializeJson(doc, requestBody);

    statusMessage = "Gondolkodom...";
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 200) {
        String response = http.getString();
        DynamicJsonDocument responseDoc(2048);
        deserializeJson(responseDoc, response);
        String gptJsonString = responseDoc["choices"][0]["message"]["content"].as<String>();

        DynamicJsonDocument resultDoc(1024);
        deserializeJson(resultDoc, gptJsonString);

        String intentType = resultDoc["type"].as<String>();

        if (intentType == "alarm") {
            alarmHour = resultDoc["hour"];
            alarmMinute = resultDoc["minute"];
            isAlarmSet = true;
            playSDSound(SOUND_ALARM_SET);
            char buff[64];
            sprintf(buff, "Ebreszto beallitva %02d:%02d-ra.", alarmHour, alarmMinute);
            statusMessage = "Ebreszto beallitva!";
            lastAIReply = String(buff);
            playTTS(String(buff));
            http.end();
            return true;
        } 
        else if (intentType == "chat") {
            String aiReply = resultDoc["reply"].as<String>();
            
            // 3. AI válaszának hozzáadása a memóriához és mentése SD-re
            JsonObject assistantMsg = messages.createNestedObject();
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = gptJsonString; // JSON-ként mentjük el, hogy a szerver értse

            File f = SD.open(CHAT_HISTORY_FILE, FILE_WRITE);
            if (f) {
                serializeJson(doc, f);
                f.close();
            }

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

//* --- SPEECH TO TEXT | PUSH TO TALK ---
void startRecordingUI() {
    playSDSound(SOUND_LISTENING);
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

    if(!client.connect("api.openai.com", 443)) {
        playSDSound(SOUND_ERROR);
        return "Hiba: Nem siikerult csatlakozni";
    } 

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
    String tail = "\r\n--" + boundary + "--\r\n";

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
    size_t chunkSize = 1024; //* sending audio data in 1024-byte chunks

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