#include "display-ui.h"

//* --- ROBOT FACE ---

void drawFace() {
    int centerX = 160;
    int centerY = 90;

    if (isRecording) {
        canvas.fillCircle(centerX - 50, centerY, 22, NAVY);
        canvas.fillCircle(centerX + 50, centerY, 22, NAVY);
        canvas.fillCircle(centerX - 50, centerY - 2, 8, WHITE);
        canvas.fillCircle(centerX + 50, centerY - 2, 8, WHITE);
    } 
    else if (statusMessage == "Gondolkodom..." || statusMessage == "Feldolgozas...") {
        canvas.fillCircle(centerX - 50, centerY - 5, 18, YELLOW);
        canvas.fillCircle(centerX + 50, centerY - 5, 18, YELLOW);
        canvas.fillCircle(centerX - 50, centerY - 10, 6, BLACK);
        canvas.fillCircle(centerX + 50, centerY - 10, 6, BLACK);
    } 
    else {
        canvas.fillCircle(centerX - 50, centerY, 18, NAVY);
        canvas.fillCircle(centerX + 50, centerY, 18, NAVY);
        canvas.fillCircle(centerX - 46, centerY - 4, 6, WHITE);
        canvas.fillCircle(centerX + 46, centerY - 4, 6, WHITE);
    }

    if (isPlayingTTS) {
        int mouthHeight = 8 + ((millis() / 120) % 4) * 6;
        canvas.fillRoundRect(centerX - 25, 135 - (mouthHeight / 2), 50, mouthHeight, 6, WHITE);
    } 
    else if (isRecording) {
        canvas.fillCircle(centerX, 135, 10, GREEN);
    } 
    else {
        canvas.fillRoundRect(centerX - 20, 135, 40, 6, 3, WHITE);
    }
}

void drawUI() {
    canvas.fillScreen(BLACK);

    if(currState == CLOCK_STATE) {
        //* --- DIGITAL CLOCK ---
        //* top bar
        struct tm timeinfo;
        if(getLocalTime(&timeinfo)) {
            if(timeinfo.tm_hour >= 23 || timeinfo.tm_hour < 6) {
                M5.Display.setBrightness(30);
            }
            else {
                M5.Display.setBrightness(70);
            }
        }
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

        //* bottom bar
        canvas.drawRoundRect(10, 185, 300, 45, 8, GREEN);
        canvas.setTextColor(GREEN);
        canvas.setTextSize(2);
        canvas.drawString("TARTSD NYOMVA A BESZEDHEZ", 160, 207);
    }
    else if(currState == CHAT_STATE) {
        //* --- CHAT BOT VIEW ---
        //* top bar
        M5.Display.setBrightness(200);
        canvas.fillRect(0, 0, 320, 25, NAVY);
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.drawString(statusMessage, 160, 12);

        //* text box
        drawFace();

        canvas.setTextDatum(BC_DATUM);
        canvas.setTextSize(1);
        canvas.setTextColor(YELLOW);

        if (lastAIReply.length() > 0) {
            // AI válasz megjelenítése a kijelző alján
            canvas.drawString(lastAIReply, 160, 230);
        } else if (lastUserSpeech.length() > 0) {
            // Felhasználói beszéd megjelenítése
            canvas.drawString(lastUserSpeech, 160, 230);
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