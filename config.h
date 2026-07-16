#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//======================================================
// AURA AI Desktop Assistant
// Hardware Configuration
// Target : ESP32-WROOM-32 (38 Pin)
// Version: 1.0.0
//======================================================

//======================================================
// PROJECT INFORMATION
//======================================================

#define AURA_NAME              "AURA"
#define AURA_VERSION           "1.0.0"

//======================================================
// OLED DISPLAY (SSD1306)
//======================================================

#define OLED_SDA_PIN           21
#define OLED_SCL_PIN           22

#define OLED_WIDTH             128
#define OLED_HEIGHT            64
#define OLED_ADDRESS           0x3C

//======================================================
// INMP441 MICROPHONE (I2S)
//======================================================

#define MIC_BCLK_PIN           26
#define MIC_WS_PIN             25
#define MIC_DATA_PIN           34

#define AUDIO_SAMPLE_RATE      16000
#define AUDIO_BITS             16

//======================================================
// WS2812 LED RING
//======================================================

#define LED_RING_PIN           4
#define LED_COUNT              16
#define LED_BRIGHTNESS         80

//======================================================
// TOUCH SENSOR
//======================================================

#define TOUCH_PIN              13
#define TOUCH_DEBOUNCE         50

//======================================================
// MICRO SD (SPI)
//======================================================

#define SD_CS_PIN              5
#define SD_MOSI_PIN            23
#define SD_MISO_PIN            19
#define SD_SCK_PIN             18

//======================================================
// WIFI SETUP PORTAL
//======================================================

#define AP_SSID                "AURA_Setup"
#define AP_PASSWORD            "12345678"

#define WIFI_TIMEOUT           30000

//======================================================
// WEB SERVER
//======================================================

#define WEB_PORT               80

//======================================================
// GOOGLE AI
//======================================================

#define GEMINI_MODEL           "gemini-2.5-flash"

#define GEMINI_URL \
"https://generativelanguage.googleapis.com/v1beta/models/" GEMINI_MODEL ":generateContent"

#define GOOGLE_STT_URL \
"https://speech.googleapis.com/v1/speech:recognize"

#define GOOGLE_TTS_URL \
"https://texttospeech.googleapis.com/v1/text:synthesize"

//======================================================
// SD CARD FILES
//======================================================

#define CONFIG_FILE            "/config.json"
#define WIFI_FILE              "/wifi.json"
#define REMINDER_FILE          "/reminders.json"
#define HISTORY_FILE           "/history.json"

#define AUDIO_FOLDER           "/audio"
#define CACHE_FOLDER           "/cache"
#define LOG_FOLDER             "/logs"

//======================================================
// LED COLORS
//======================================================

#define LED_BOOT               0xFFFFFF
#define LED_READY              0x00FF00
#define LED_LISTENING          0x0000FF
#define LED_PROCESSING         0xFFFF00
#define LED_SPEAKING           0x8000FF
#define LED_MUTED              0xFF0000
#define LED_ERROR              0xFF3300

//======================================================
// SYSTEM STATES
//======================================================

enum AuraState
{
    BOOTING,
    CONNECTING_WIFI,
    READY,
    LISTENING,
    PROCESSING,
    SPEAKING,
    MIC_MUTED,
    SETTINGS_MODE,
    OTA_UPDATE,
    ERROR_STATE
};

//======================================================
// LOG LEVEL
//======================================================

enum LogLevel
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

#define CURRENT_LOG_LEVEL LOG_INFO

#endif

