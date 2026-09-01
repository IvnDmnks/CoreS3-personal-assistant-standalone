#pragma once

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
#include <SD.h>
#include <SPI.h>
#include "AudioFileSourceSD.h"

//* -----------------------------------
//* --- KONSTANSOK ÉS FAJLOK (SOUNDS) ---
//* -----------------------------------
#define SD_CS_PIN 4

#define SOUND_ALARM_SET "/sounds/alarm_set.mp3"
#define SOUND_LISTENING "/sounds/listening.mp3"
#define SOUND_ERROR     "/sounds/error.mp3"
#define SOUND_WIFI_OK   "/sounds/wifi_ok.mp3"
#define SOUND_WIFI_ERR  "/sounds/wifi_err.mp3"

//* ---------------
//* --- GLOBALS ---
//* ---------------

extern const char* CITY;
extern const char* NTP_SERVER;
extern const long  GMT_OFFSET_SEC;
extern const int   DAYLIGHT_OFFSET_SEC;

extern WiFiMulti wifiMulti;

static constexpr const size_t record_number = 512;
static constexpr const size_t record_length = 320;
static constexpr const size_t record_size = record_number * record_length;
static constexpr const size_t record_samplerate = 16000;
extern int16_t *rec_data;
extern size_t rec_record_idx;

extern bool isAlarmSet;
extern int alarmHour;
extern int alarmMinute;
extern bool isRinging;
extern bool isPlayingTTS;
extern bool isRecording;

enum UIState {
    CLOCK_STATE,
    CHAT_STATE
};

extern UIState currState;
extern M5Canvas canvas;

extern String lastUserSpeech;
extern String lastAIReply;
extern String statusMessage;
extern unsigned long chatStateStartTime;

extern AudioGeneratorMP3 *mp3;
extern AudioFileSourceHTTPStream *file;
extern AudioOutputI2S *out;

extern const char* CHAT_HISTORY_FILE;