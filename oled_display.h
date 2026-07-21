#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "logger.h"
#include "utilities.h"

/**
 * @class OledDisplay
 * @brief Manages the SSD1306 OLED display used by AURA.
 *
 * Provides screen rendering for AURA system states, Wi-Fi status, reminders,
 * OTA update progress, errors, and custom messages.
 */
class OledDisplay {
public:
    /**
     * @brief Constructs the OLED display manager.
     */
    OledDisplay();

    ~OledDisplay() = default;

    OledDisplay(const OledDisplay&) = delete;
    OledDisplay& operator=(const OledDisplay&) = delete;
    OledDisplay(OledDisplay&&) = delete;
    OledDisplay& operator=(OledDisplay&&) = delete;

    /**
     * @brief Initializes the SSD1306 display over I2C.
     * @return True when the display initializes successfully; otherwise false.
     */
   bool initialize();

/**
 * @brief OLED display task loop.
 *
 * Processes display updates and animations when running under FreeRTOS.
 */
void run();

/**
 * @brief Clears the display framebuffer.
 */
void clear();

    /**
     * @brief Transfers the framebuffer contents to the OLED panel.
     */
    void update();

    /**
     * @brief Sets the OLED contrast level.
     * @param brightness Contrast value in the range 0 to 255.
     */
    void setBrightness(uint8_t brightness);

    /**
     * @brief Displays the AURA startup screen.
     */
    void showBootScreen();

    /**
     * @brief Displays the ready and idle screen.
     */
    void showReadyScreen();

    /**
     * @brief Displays the voice-listening screen.
     */
    void showListeningScreen();

    /**
     * @brief Displays the request-processing screen.
     */
    void showThinkingScreen();

    /**
     * @brief Displays the response-speaking screen.
     */
    void showSpeakingScreen();

    /**
     * @brief Displays the Wi-Fi connection-in-progress screen.
     */
    void showWifiConnecting();

    /**
     * @brief Displays the successful Wi-Fi connection screen.
     */
    void showWifiConnected();

    /**
     * @brief Displays a reminder message.
     * @param reminder Reminder text to render.
     */
    void showReminder(const String& reminder);

    /**
     * @brief Displays OTA firmware update progress.
     * @param percent Update completion percentage, from 0 to 100.
     */
    void showOTAUpdate(uint8_t percent);

    /**
     * @brief Displays an error message.
     * @param message Error description to render.
     */
    void showError(const String& message);

    /**
     * @brief Displays text horizontally centered on the OLED.
     * @param text Text to render.
     */
    void showCenteredText(const String& text);

    /**
     * @brief Displays a titled message screen.
     * @param title Title text.
     * @param body Message body text.
     */
    void showMessage(const String& title, const String& body);

    /**
     * @brief Displays the screen associated with the supplied AURA state.
     * @param state Current AURA operating state.
     */
    void showStatus(AuraState state);

private:
    Adafruit_SSD1306 display;

    /**
     * @brief Draws text centered horizontally at a specified vertical position.
     * @param text Text to draw.
     * @param y Vertical position in pixels.
     * @param textSize Display text scale factor.
     */
    void drawCenteredString(
        const String& text,
        int16_t y,
        uint8_t textSize = 1U);

    /**
     * @brief Draws a header title at the top of the display.
     * @param title Header title text.
     */
    void drawHeader(const String& title);

    /**
     * @brief Draws footer text at the bottom of the display.
     * @param text Footer text.
     */
    void drawFooter(const String& text);

    /**
     * @brief Draws a bordered horizontal progress bar.
     * @param x Left position in pixels.
     * @param y Top position in pixels.
     * @param width Bar width in pixels.
     * @param height Bar height in pixels.
     * @param percent Fill percentage, from 0 to 100.
     */
    void drawProgressBar(
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        uint8_t percent);
};

#endif // OLED_DISPLAY_H