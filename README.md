# M5Stack CoreS3 Standalone AI Assistant

An all-in-one, fully standalone desktop AI assistant and smart clock powered by the **M5Stack CoreS3 (ESP32-S3)**. Featuring a touchscreen display, onboard microphone, speaker, and 6-axis IMU, the device communicates directly with REST APIs, OpenAI services (Whisper, GPT-4o-mini, TTS), and Apple CalDAV servers over Wi-Fi without requiring a host computer or local server.

## Key Features

* **100% Standalone:** Runs entirely on-device; no local server, Raspberry Pi, or host scripts needed.
* **Push-to-Talk Voice Input (Whisper API):** Hold to record high-quality audio using the onboard I2S microphone and stream WAV data directly to OpenAI Whisper for speech-to-text.
* **Smart AI Intent Routing & Chat:** Automatically distinguishes between setting alarms and answering general questions using GPT-4o-mini, featuring real-time visual chat rendering.
* **Dynamic Dual-UI Display:** Seamlessly switches between a sleek digital clock (with date and alarm status) and an interactive Chat UI using flicker-free `M5Canvas` sprites.
* **Table Knock Snooze (IMU Sensor):** Uses the built-in 6-axis accelerometer to detect physical knocks/taps on the desk to turn off or snooze the alarm and initiate the morning briefing.
* **Morning Briefing Routine:** Fuses multi-calendar events (Apple CalDAV) and local weather (OpenWeatherMap API) into a friendly, voice-synthesized daily briefing.
* **Multi-Wi-Fi Roaming (`WiFiMulti`):** Automatically connects to the strongest available saved network (e.g., Home Wi-Fi, Mobile Hotspot, Office) for seamless travel.
* **I2S Audio Playback:** Synthesizes and plays back speech directly through the CoreS3's onboard AW88298 amplifier and speaker.

## Hardware & Software Requirements

* **Hardware:** [M5Stack CoreS3](https://docs.m5stack.com/en/core/CoreS3) (ESP32-S3 with Touchscreen, Mic, Speaker, IMU)
* **Development Environment:** VS Code + [PlatformIO IDE](https://platformio.org/) extension

## Getting Started

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/your-username/cores3-standalone.git](https://github.com/your-username/cores3-standalone.git)
   cd cores3-standalone
   ```

2. **Configure your secrets:**
   Copy the template file `src/secrets-example.h` to `src/secrets.h`:
   ```bash
   cp src/secrets-example.h src/secrets.h
   ```
   Open `src/secrets.h` and fill in your credentials:
   * Multiple Wi-Fi networks (SSIDs & Passwords) for `WiFiMulti` roaming.
   * OpenAI API Key (Whisper, GPT-4o-mini, TTS).
   * OpenWeatherMap API Key & City.
   * Apple ID App-Specific Password & iCloud CalDAV URLs.

3. **Build & Flash:**
   * Connect your M5Stack CoreS3 via USB-C to your computer.
   * Open the project in VS Code with PlatformIO.
   * Click **Build** (`Ctrl+Alt+B` / `Cmd+Option+B`) and then **Upload** to flash the firmware.

## Project Structure

```text
cores3-standalone/
├── platformio.ini       # PlatformIO build settings & library dependencies
├── .gitignore           # Git ignore rules (secrets.h, build artifacts)
├── README.md            # Project documentation
└── src/
    ├── main.cpp          # Main C++ source (WiFiMulti, UI Canvas, IMU, Whisper, GPT, TTS)
    ├── secrets-example.h # Configuration template
    └── secrets.h        # Local credentials (ignored by Git)
```
---
README.md created by Gemini.