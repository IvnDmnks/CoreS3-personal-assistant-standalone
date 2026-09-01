#pragma once
#include "globals.h"

String getAppleAuthHeader();
String get_weather();
String get_today_events();
String getGPTSummary(String systemRole, String userContext);
void clearChatHistory();
bool processIntent(String text);
void startRecordingUI();
void stopRecordingUI();
void createWavHeader(byte* header, uint32_t waveDataSize);
String sendAudioToWhisper(byte* wavHeader, uint32_t recorded_bytes);