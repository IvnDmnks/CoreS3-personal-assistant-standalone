#pragma once

// --- WI-FI SETTINGS ---
const char* WIFI_SSID1 = "YOUR_WIFI_SSID";
const char* WIFI_PASS1 = "YOUR_WIFI_PASSWORD";

const char* WIFI_SSID2 = "YOUR_WIFI_SSID";
const char* WIFI_PASS2 = "YOUR_WIFI_PASSWORD";

// --- API KEYS ---
const char* OPENAI_API_KEY = "sk-proj-your_openai_api_key";
const char* OPENWEATHER_API_KEY = "your_openweather_api_key";

// --- APPLE ICLOUD CALDAV ---
const char* APPLE_ID = "your.apple.id@icloud.com";
const char* APPLE_APP_PASSWORD = "abcd-efgh-ijkl-mnop";

// --- ICLOUD CALENDAR URLS ---
// Add your iCloud CalDAV calendar URLs below
const char* CALENDAR_URLS[] = {
    "https://p123-caldav.icloud.com/1234567/calendars/work/",
    "https://p123-caldav.icloud.com/1234567/calendars/personal/"
};

// Automatic array element counter
const int NUM_CALENDARS = sizeof(CALENDAR_URLS) / sizeof(CALENDAR_URLS[0]);