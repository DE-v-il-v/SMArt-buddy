#ifndef AURA_DISPLAY_MANAGER_H
#define AURA_DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cstdint>
#include "config.h"
#include "logger.h"

/**
 * @enum DisplayState
 * @brief Enumeration of display states
 */
enum class DisplayState : uint8_t {
    BOOT,          ///< Boot/initialization screen
    HOME,          ///< Home/idle screen
    LISTENING,     ///< Microphone listening state
    THINKING,      ///< Processing/thinking state
    SPEAKING,      ///< Speaker output state
    REMINDER,      ///< Reminder notification
    NOTIFICATION,  ///< Generic notification
    ERROR,         ///< Error display
    OTA,           ///< Over-the-air update progress
    SLEEP          ///< Sleep/low-power state
};

/**
 * @class DisplayManager
 * @brief Single authority for all OLED display operations
 *
 * Manages:
 * - OLED initialization and control
 * - State-based screen rendering
 * - Animations and transitions
 * - Status displays (Wi-Fi, storage, system)
 * - Notifications and error messages
 * - Brightness and contrast control
 * - Sleep mode management
 * - Display refresh and buffering
 *
 * Non-blocking, production-quality display management for ESP32.
 */
class DisplayManager {
public:
    /**
     * @brief Constructor
     */
    DisplayManager() noexcept;

    /**
     * @brief Destructor
     */
    ~DisplayManager() noexcept;

    // Delete copy semantics
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    // Delete move semantics
    DisplayManager(DisplayManager&&) = delete;
    DisplayManager& operator=(DisplayManager&&) = delete;

    /**
     * @brief Initialize the display manager
     * @return true if initialization successful, false otherwise
     * @note Should be called once during setup()
     */
    [[nodiscard]] bool initialize() noexcept;

    /**
     * @brief Update display state and rendering
     * @note Should be called regularly from loop()
     * @note Non-blocking; respects refresh interval
     */
    void update() noexcept;

    /**
     * @brief Scheduler-compatible alias for update()
     */
    void run() noexcept;

    /**
     * @brief Clear the display buffer
     */
    void clear() noexcept;
    /**
 * @brief Reset the display manager to its default state.
 */
void reset() noexcept;

    /**
     * @brief Refresh/update the display with buffer contents
     */
    void refresh() noexcept;

    /**
     * @brief Force immediate redraw of current state
     * @note Use only when display must update immediately
     */
    void forceRefresh() noexcept;

    // ========================================================================
    // State Management
    // ========================================================================

    /**
     * @brief Change display state
     * @param newState Target display state
     * @note Preferred method for screen transitions
     */
    void setState(DisplayState newState) noexcept;

    /**
     * @brief Get current display state
     * @return Current DisplayState value
     */
    [[nodiscard]] DisplayState getState() const noexcept;

    /**
     * @brief Get display initialization status
     * @return true if initialized, false otherwise
     */
    [[nodiscard]] bool isInitialized() const noexcept;

    // ========================================================================
    // Screen Display Methods
    // ========================================================================

    /**
     * @brief Show AURA splash screen
     */
    void showSplash() noexcept;

    /**
     * @brief Show boot/initialization screen with progress
     * @param progress Progress percentage (0-100)
     */
    void showBoot(uint8_t progress) noexcept;

    /**
     * @brief Show home/idle screen
     */
    void showHome() noexcept;

    /**
     * @brief Show microphone listening state
     */
    void showListening() noexcept;

    /**
     * @brief Show processing/thinking state
     */
    void showThinking() noexcept;

    /**
     * @brief Show speaker output/speaking state
     */
    void showSpeaking() noexcept;

    /**
     * @brief Show reminder notification
     * @param title Reminder title text
     * @param message Reminder message text
     */
    void showReminder(const String& title, const String& message) noexcept;

    /**
     * @brief Show generic notification popup
     * @param title Notification title
     * @param message Notification message
     * @param durationMs Display duration in milliseconds (0 = indefinite)
     */
    void showNotification(const String& title, const String& message, unsigned long durationMs = 0) noexcept;

    /**
     * @brief Show error message
     * @param title Error title
     * @param message Error description
     */
    void showError(const String& title, const String& message) noexcept;

    /**
     * @brief Show OTA (Over-The-Air) update progress
     * @param progress Update progress percentage (0-100)
     */
    void showOTAProgress(uint8_t progress) noexcept;

    /**
     * @brief Show Wi-Fi status screen
     * @param connected true if connected, false if disconnected
     * @param ssid Network SSID name
     * @param signal Signal strength (RSSI in dBm, or 0)
     */
    void showWifiStatus(bool connected, const String& ssid, int32_t signal) noexcept;

    /**
     * @brief Show storage status screen
     * @param storageType Storage media name (e.g., "SPIFFS", "SD Card")
     * @param usedMB Used storage in MB
     * @param totalMB Total storage in MB
     */
    void showStorageStatus(const String& storageType, uint32_t usedMB, uint32_t totalMB) noexcept;

    /**
     * @brief Show custom message screen
     * @param title Message title
     * @param body Message body text
     * @param footer Optional footer text
     */
    void showMessage(const String& title, const String& body, const String& footer = "") noexcept;

    // ========================================================================
    // Display Control Methods
    // ========================================================================

    /**
     * @brief Set display brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness) noexcept;

    /**
     * @brief Enter sleep mode (blank display, reduced power)
     */
    void sleep() noexcept;

    /**
     * @brief Wake from sleep mode (restore previous display)
     */
    void wake() noexcept;

/**
     * @brief Turn OLED panel on.
     */
    void displayOn() noexcept;

    /**
     * @brief Turn OLED panel off.
     */
    void displayOff() noexcept;

    /**
     * @brief Check if display is in sleep mode
     * @return true if sleeping, false otherwise
     */
    [[nodiscard]] bool isSleeping() const noexcept;

    /**
     * @brief Check if display is awake
     * @return true if awake, false if sleeping
     */
    [[nodiscard]] bool isAwake() const noexcept;

    /**
     * @brief Set display rotation
     * @param rotation Rotation angle (0, 1, 2, 3)
     */
    void setRotation(uint8_t rotation) noexcept;

    /**
     * @brief Set display contrast
     * @param contrast Contrast level (0-255)
     */
    void setContrast(uint8_t contrast) noexcept;

    /**
     * @brief Set display color inversion
     * @param inverted true to invert colors, false for normal
     */
    void setInverted(bool inverted) noexcept;

    // ========================================================================
    // Display Information Methods
    // ========================================================================

    /**
     * @brief Get display width in pixels
     * @return Width in pixels (typically 128)
     */
    [[nodiscard]] uint16_t getWidth() const noexcept;

    /**
     * @brief Get display height in pixels
     * @return Height in pixels (typically 64)
     */
    [[nodiscard]] uint16_t getHeight() const noexcept;

private:
    // Private state management
    void changeState(DisplayState newState) noexcept;

    // Private display rendering methods
    void renderBoot(uint8_t progress) noexcept;
    void renderHome() noexcept;
    void renderListening() noexcept;
    void renderThinking() noexcept;
    void renderSpeaking() noexcept;
    void renderReminder() noexcept;
    void renderNotification() noexcept;
    void renderError() noexcept;
    void renderOTAProgress() noexcept;
    void renderWifiStatus() noexcept;
    void renderStorageStatus() noexcept;
    void renderMessage() noexcept;
    void renderSleep() noexcept;

    // Private helper methods
    void drawCenteredText(const String& text, uint8_t y, uint8_t textSize = 1) noexcept;
    void drawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t percent) noexcept;
    void drawWifiIcon(uint8_t x, uint8_t y, int32_t signal) noexcept;
    void drawStorageIcon(uint8_t x, uint8_t y) noexcept;
    void drawStatusIcons() noexcept;
    void updateAnimation() noexcept;
    void updateScreenTimeout() noexcept;

    // Private member variables
    Adafruit_SSD1306 m_display;              ///< OLED display object
    DisplayState m_currentState;             ///< Current display state
    DisplayState m_previousState;            ///< Previous display state
    DisplayState m_lastRenderedState;
    bool m_initialized;                      ///< Initialization state
    bool m_sleeping;                         ///< Sleep mode state
    bool m_screenDirty;                      ///< Screen needs redraw flag
    unsigned long m_lastUpdateTime;          ///< Last update timestamp
    unsigned long m_lastRefreshTime;         ///< Last refresh timestamp
    unsigned long m_stateStartTime;          ///< Time when state changed
    unsigned long m_notificationTimeout;     ///< Notification display timeout
    unsigned long m_lastActivityTime;        ///< Last user/system activity time
    uint8_t m_brightness;                    ///< Current brightness level
    uint8_t m_contrast;                      ///< Current contrast level
    bool m_inverted;                         ///< Display inversion state
    uint8_t m_rotation;                      ///< Display rotation (0-3)
    uint32_t m_animationFrame;               ///< Animation frame counter

    // Cached display data
    String m_cachedTitle;                    ///< Cached title text
    String m_cachedMessage;                  ///< Cached message text
    String m_cachedFooter;                   ///< Cached footer text
    String m_cachedSSID;                     ///< Cached Wi-Fi SSID
    String m_cachedStorageType;              ///< Cached storage type name
    bool m_cachedWifiConnected;              ///< Cached Wi-Fi connection state
    int32_t m_cachedSignal;                  ///< Cached signal strength
    uint32_t m_cachedUsedMB;                 ///< Cached used storage in MB
    uint32_t m_cachedTotalMB;                ///< Cached total storage in MB
    uint8_t m_cachedOTAProgress;             ///< Cached OTA progress percentage
    uint8_t m_cachedBootProgress;            ///< Cached boot progress percentage

    // Display configuration constants
    static constexpr uint16_t m_displayWidth{OLED_WIDTH};           ///< Display width in pixels
    static constexpr uint16_t m_displayHeight{OLED_HEIGHT};         ///< Display height in pixels
    static constexpr unsigned long m_screenTimeoutMs{30000UL};      ///< Screen sleep timeout (30 seconds)
    static constexpr unsigned long m_refreshIntervalMs{33UL};       ///< Refresh interval (≈30 FPS)
    static constexpr unsigned long m_animationIntervalMs{100UL};
    static constexpr uint8_t m_defaultBrightness{200};              ///< Default brightness level
    static constexpr uint8_t m_defaultContrast{255};
};

/**
 * @brief Global display manager instance
 */
extern DisplayManager displayManager;

#endif // AURA_DISPLAY_MANAGER_H