# M5Stack CoreS3 Standalone AI Assistant 🤖☀️

An all-in-one, fully standalone desktop AI assistant with a touchscreen display and built-in speaker, powered by the **M5Stack CoreS3 (ESP32-S3)**. The microcontroller communicates directly with REST APIs and Apple CalDAV servers over Wi-Fi without needing a host computer or backend server.

## 🌟 Key Features

* **100% Standalone:** Runs entirely on-device; no local server or running Python scripts required.
* **Multi-iCloud Calendar Support:** Queries multiple Apple CalDAV calendars in parallel using direct HTTP `REPORT` requests.
* **Live Weather Updates:** Integrates with the OpenWeatherMap API for local weather forecasts.
* **GPT-4o-mini Context Summarization:** Fuses daily events and weather data into a friendly, natural morning brief.
* **I2S Audio Streaming:** Plays back OpenAI Text-to-Speech audio directly through the CoreS3's onboard AW88298 I2S amplifier and speaker.
* **Touch Screen Trigger:** Tap the screen to trigger the morning briefing on demand.

## 🛠️ Hardware & Software Requirements

* **Hardware:** [M5Stack CoreS3](https://docs.m5stack.com/en/core/CoreS3) (ESP32-S3)
* **Development Environment:** VS Code + [PlatformIO IDE](https://platformio.org/) extension

## 🚀 Getting Started

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
   Open `src/secrets.h` and fill in your credentials (Wi-Fi SSID, Passwords, API Keys, Apple Credentials, and iCloud CalDAV URLs).

3. **Build & Flash:**
   * Connect your M5Stack CoreS3 via USB-C to your computer.
   * Open the project in VS Code with PlatformIO.
   * Click **Build** (`Ctrl+Alt+B` / `Cmd+Option+B`) and then **Upload** to flash the firmware.

## 📁 Project Structure

```text
cores3-standalone/
├── platformio.ini       # PlatformIO build settings & dependencies
├── .gitignore           # Git ignore rules (secrets.h, build artifacts)
├── README.md
└── src/
    ├── main.cpp          # Main C++ source (WiFi, UI, CalDAV, GPT, TTS)
    ├── secrets-example.h # Configuration template
    └── secrets.h        # Local credentials (ignored by Git)
```