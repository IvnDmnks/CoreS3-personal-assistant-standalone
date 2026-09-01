#include "globals.h"

const char* CITY = "Budapest";
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 3600;
const int   DAYLIGHT_OFFSET_SEC = 3600;

WiFiMulti wifiMulti;

int16_t *rec_data = nullptr;
size_t rec_record_idx  = 2;

bool isAlarmSet = false;
int alarmHour = 0;
int alarmMinute = 0;
bool isRinging = false;
bool isPlayingTTS = false;
bool isRecording = false;

UIState currState = CLOCK_STATE;

M5Canvas canvas(&M5.Display);

String lastUserSpeech = "";
String lastAIReply = "";
String statusMessage = "";

unsigned long chatStateStartTime = 0;

AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceHTTPStream *file = nullptr;
AudioOutputI2S *out = nullptr;

const char* CHAT_HISTORY_FILE = "/chat_history.json";