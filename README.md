# M5Stack CoreS3 Standalone AI Assistant

An all-in-one, fully standalone desktop AI assistant and smart clock powered by the **M5Stack CoreS3 (ESP32-S3)**. Featuring a touchscreen display, onboard microphone, speaker, MicroSD audio storage, and a 6-axis IMU, the device communicates directly with REST APIs, OpenAI services (Whisper, GPT-4o-mini, TTS), and Apple CalDAV servers over Wi-Fi without requiring a host computer or local server.

## Key Features

* **100% Standalone:** Runs entirely on-device; no local server, Raspberry Pi, or host scripts needed.
* **Push-to-Talk Voice Input (Whisper API):** Hold to record high-quality audio using the onboard I2S microphone and stream WAV data directly to OpenAI Whisper for speech-to-text.
* **Smart AI Intent Routing & Chat:** Automatically distinguishes between setting alarms and answering general questions using GPT-4o-mini, featuring real-time visual chat rendering.
* **Dynamic Dual-UI Display:** Seamlessly switches between a sleek digital clock (with date and alarm status) and an interactive Chat UI using flicker-free `M5Canvas` sprites.
* **Modular Architecture:** Clean C++ structure split across dedicated modules (AI Assistant, Audio Controller, Alarm Manager, Display UI, Globals) for robust maintainability.
* **Local SD Sound Effects & TTS Playback:** Plays system feedback sounds (`/sounds/` on SD) and streams/downloads OpenAI synthesized speech through the onboard AW88298 amplifier.
* **Table Knock Snooze (IMU Sensor):** Uses the built-in 6-axis accelerometer to detect physical knocks/taps on the desk to turn off or snooze the alarm and initiate the morning briefing.
* **Morning Briefing Routine:** Fuses multi-calendar events (Apple CalDAV) and local weather (OpenWeatherMap API) into a friendly, voice-synthesized daily briefing.
* **Multi-Wi-Fi Roaming (`WiFiMulti`):** Automatically connects to the strongest available saved network (e.g., Home Wi-Fi, Mobile Hotspot, Office) for seamless travel.

## Hardware & Software Requirements

* **Hardware:** [M5Stack CoreS3](https://docs.m5stack.com/en/core/CoreS3) (ESP32-S3 with Touchscreen, Mic, Speaker, IMU)
* **Storage:** MicroSD Card (formatted as FAT32)
* **Development Environment:** VS Code + [PlatformIO IDE](https://platformio.org/) extension

## Getting Started

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/your-username/cores3-standalone.git](https://github.com/your-username/cores3-standalone.git)
   cd cores3-standalone
   ```

2. **Prepare the MicroSD Card:**
   * Format your MicroSD card to **FAT32**.
   * Create a `/sounds/` folder in the root directory.
   * Copy the required sound assets into `/sounds/`:
     * `alarm_set.mp3`
     * `listening.mp3`
     * `error.mp3`
     * `wifi_ok.mp3`
     * `wifi_err.mp3`
     * `alarm.wav`
   * Insert the MicroSD card into your M5Stack CoreS3.

3. **Configure your secrets:**
   Copy the template file `src/secrets-example.h` to `src/secrets.h`:
   ```bash
   cp src/secrets-example.h src/secrets.h
   ```
   Open `src/secrets.h` and fill in your macros (ensure no `=` or `;` at the end of `#define` lines):
   * Wi-Fi network credentials (`WIFI_SSID1`, `WIFI_PASS1`, etc.).
   * OpenAI API Key (`OPENAI_API_KEY`).
   * OpenWeatherMap API Key & City (`OPENWEATHER_API_KEY`, `CITY`).
   * Apple ID App-Specific Password & CalDAV URLs (`APPLE_ID`, `APPLE_APP_PASSWORD`, `CALENDAR_URLS`).

4. **Build & Flash:**
   * Connect your M5Stack CoreS3 via USB-C to your computer.
   * Open the project in VS Code with PlatformIO.
   * Click **Build** (`Ctrl+Alt+B` / `Cmd+Option+B`) and then **Upload** to flash the firmware.

## Project Structure

```text
cores3-standalone/
├── platformio.ini            # PlatformIO build settings & library dependencies
├── .gitignore                # Git ignore rules (secrets.h, build artifacts)
├── README.md                 # Project documentation
└── src/
    ├── main.cpp               # Core setup(), loop(), and task orchestration
    ├── globals.h / .cpp       # Shared variables, state enums, and pin definitions
    ├── ai-assistant.h / .cpp  # OpenAI REST calls (Whisper, GPT-4o-mini), Weather & CalDAV
    ├── audio-controller.h / .cpp # Audio handling (SD sound playback, TTS downloader/player)
    ├── alarm-manager.h / .cpp # Alarm clock logic, storage persistence, and trigger checks
    ├── display-ui.h / .cpp    # Canvas rendering logic (Clock UI & Chat UI)
    ├── secrets-example.h      # Configuration template macro file
    └── secrets.h              # Local credentials (ignored by Git)
```
