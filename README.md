# AURA AI Desktop Assistant

> **A modular, ESP32-powered AI voice assistant inspired by JARVIS.**

## Features

-   Google Gemini 2.5 Flash
-   Google Cloud STT & TTS
-   British male neural voice
-   Web configuration portal
-   SD card support
-   Reminder manager
-   OLED display
-   OTA framework
-   Non-blocking state machine architecture

## Hardware

  Component    Model
  ------------ ----------------
  MCU          ESP32-WROOM-32
  Display      SSD1306 OLED
  Microphone   INMP441
  Speaker      MAX98357
  Storage      MicroSD

## Build Requirements

-   Arduino IDE 2.x
-   ESP32 Arduino Core 3.3.10
-   C++17

### Recommended Settings

-   Board: ESP32 Dev Module
-   Partition: Huge APP (3MB No OTA / 1MB SPIFFS)
-   CPU: 240 MHz

## Configuration


``` cpp
#pragma once
#define WIFI_SSID      "YOUR_WIFI"
#define WIFI_PASSWORD  "YOUR_PASSWORD"
#define GEMINI_API_KEY "YOUR_GEMINI_KEY"
#define GOOGLE_STT_KEY "YOUR_STT_KEY"
#define GOOGLE_TTS_KEY "YOUR_TTS_KEY"
```

## Roadmap

### v0.9.8-beta

-   Modular firmware
-   Gemini integration
-   STT/TTS
-   Web portal
-   SD card support

### v1.0.0

-   Stable release


## License

MIT

## Author

**Devil**

**Project:** AURA AI Desktop Assistant

**Codename:** Phoenix
