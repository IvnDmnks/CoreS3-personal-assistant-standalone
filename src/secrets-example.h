#pragma once

// --- WI-FI SETTINGS ---
#define WIFI_SSID1 "YOUR_WIFI_SSID"
#define WIFI_PASS1 "YOUR_WIFI_PASSWORD"

#define WIFI_SSID2 "YOUR_WIFI_SSID"
#define WIFI_PASS2 "YOUR_WIFI_PASSWORD"

// --- API KEYS ---
#define OPENAI_API_KEY "sk-proj-your_openai_api_key"
#define OPENWEATHER_API_KEY "your_openweather_api_key"

// --- APPLE ICLOUD CALDAV ---
#define APPLE_ID "your.apple.id@icloud.com"
#define APPLE_APP_PASSWORD "abcd-efgh-ijkl-mnop"

// --- ICLOUD CALENDAR URLS ---
// Add your iCloud CalDAV calendar URLs below
static const char* const CALENDAR_URLS[] = {
    "https://p123-caldav.icloud.com/1234567/calendars/work/",
    "https://p123-caldav.icloud.com/1234567/calendars/personal/"
};

// Automatic array element counter
static const int NUM_CALENDARS = sizeof(CALENDAR_URLS) / sizeof(CALENDAR_URLS[0]);