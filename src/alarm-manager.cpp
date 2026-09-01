#include "alarm-manager.h"
#include "audio-controller.h"
#include "ai-assistant.h"
#include "display-ui.h"

//* --- ALARM CLOCK ---
void checkAlarm() {
    if(!isAlarmSet || isRinging) return;
    
    struct tm timeInfo;
    if(getLocalTime(&timeInfo)) {
        if(timeInfo.tm_hour == alarmHour && timeInfo.tm_min == alarmMinute) {
            isAlarmSet = false; //* alarm set off, otherwise morning routine would run 60 times per minute
            isRinging = true;
            M5.Speaker.begin();
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