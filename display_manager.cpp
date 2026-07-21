#include "display_manager.h"
#include <Wire.h>

DisplayManager displayManager;

DisplayManager::DisplayManager() noexcept
    : m_display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET)
    , m_currentState(DisplayState::BOOT)
    , m_previousState(DisplayState::BOOT)
    , m_lastRenderedState(DisplayState::BOOT)
    , m_initialized(false)
    , m_sleeping(false)
    , m_screenDirty(true)
    , m_lastUpdateTime(0)
    , m_lastRefreshTime(0)
    , m_stateStartTime(0)
    , m_notificationTimeout(0)
    , m_lastActivityTime(0)
    , m_brightness(m_defaultBrightness)
    , m_contrast(m_defaultContrast)
    , m_inverted(false)
    , m_rotation(0)
    , m_animationFrame(0)
    , m_cachedTitle("")
    , m_cachedMessage("")
    , m_cachedFooter("")
    , m_cachedSSID("")
    , m_cachedStorageType("")
    , m_cachedWifiConnected(false)
    , m_cachedSignal(0)
    , m_cachedUsedMB(0)
    , m_cachedTotalMB(0)
    , m_cachedOTAProgress(0)
    , m_cachedBootProgress(0) {}

DisplayManager::~DisplayManager() noexcept = default;

bool DisplayManager::initialize() noexcept {
    if (m_initialized) {
        return true;
    }

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!m_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        LOG_ERROR("DisplayManager", "SSD1306 initialization failed");
        return false;
    }

    m_display.clearDisplay();
    m_display.setTextColor(SSD1306_WHITE);
    m_display.setTextWrap(false);
    m_display.cp437(true);

    // Mark initialized before calling setter functions
m_initialized = true;

    setBrightness(m_brightness);
    setContrast(m_contrast);
    setRotation(m_rotation);
    setInverted(m_inverted);

    
    m_lastUpdateTime = millis();
    m_lastRefreshTime = millis();
    m_stateStartTime = millis();
    m_lastActivityTime = millis();

    LOG_INFO("DisplayManager", "DisplayManager initialized");
    return true;
}

void DisplayManager::run() noexcept {
    update();
}

void DisplayManager::update() noexcept {
    if (!m_initialized || m_sleeping) {
        return;
    }

    const unsigned long now = millis();

    updateAnimation();
    updateScreenTimeout();

    if (m_screenDirty || (now - m_lastRefreshTime >= m_refreshIntervalMs)) {
        if (m_currentState != m_lastRenderedState || m_screenDirty) {
            switch (m_currentState) {
                case DisplayState::BOOT:
                    renderBoot(m_cachedBootProgress);
                    break;
                case DisplayState::HOME:
                    renderHome();
                    break;
                case DisplayState::LISTENING:
                    renderListening();
                    break;
                case DisplayState::THINKING:
                    renderThinking();
                    break;
                case DisplayState::SPEAKING:
                    renderSpeaking();
                    break;
                case DisplayState::REMINDER:
                    renderReminder();
                    break;
                case DisplayState::NOTIFICATION:
                    renderNotification();
                    break;
                case DisplayState::ERROR:
                    renderError();
                    break;
                case DisplayState::OTA:
                    renderOTAProgress();
                    break;
                case DisplayState::SLEEP:
                    renderSleep();
                    break;
            }
            m_lastRenderedState = m_currentState;
            m_screenDirty = false;
        }
        m_display.display();
        m_lastRefreshTime = now;
    }
}

void DisplayManager::clear() noexcept {
    if (!m_initialized) return;
    m_display.clearDisplay();
    m_screenDirty = true;
}

void DisplayManager::reset() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::BOOT);
    m_cachedTitle.clear();
    m_cachedMessage.clear();
    m_cachedFooter.clear();
    m_cachedSSID.clear();
    m_cachedStorageType.clear();
    m_cachedWifiConnected = false;
    m_cachedSignal = 0;
    m_cachedUsedMB = 0;
    m_cachedTotalMB = 0;
    m_cachedOTAProgress = 0;
    m_cachedBootProgress = 0;
    m_notificationTimeout = 0;
    m_animationFrame = 0;
    m_screenDirty = true;
}

void DisplayManager::refresh() noexcept {
    if (!m_initialized || m_sleeping) return;
    m_display.display();
}

void DisplayManager::forceRefresh() noexcept {
    if (!m_initialized) return;
    m_screenDirty = true;
    m_lastRenderedState = static_cast<DisplayState>(255);
    update();
}

void DisplayManager::setState(DisplayState newState) noexcept {
    changeState(newState);
}

DisplayState DisplayManager::getState() const noexcept {
    return m_currentState;
}

bool DisplayManager::isInitialized() const noexcept {
    return m_initialized;
}

void DisplayManager::showSplash() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::BOOT);
    m_cachedBootProgress = 0;
    m_screenDirty = true;
}

void DisplayManager::showBoot(uint8_t progress) noexcept {
    if (!m_initialized) return;
    if (m_currentState != DisplayState::BOOT) {
        changeState(DisplayState::BOOT);
    }
    m_cachedBootProgress = progress;
    m_screenDirty = true;
}

void DisplayManager::showHome() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::HOME);
    m_screenDirty = true;
}

void DisplayManager::showListening() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::LISTENING);
    m_screenDirty = true;
}

void DisplayManager::showThinking() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::THINKING);
    m_screenDirty = true;
}

void DisplayManager::showSpeaking() noexcept {
    if (!m_initialized) return;
    changeState(DisplayState::SPEAKING);
    m_screenDirty = true;
}

void DisplayManager::showReminder(const String& title, const String& message) noexcept {
    if (!m_initialized) return;
    m_cachedTitle = title;
    m_cachedMessage = message;
    changeState(DisplayState::REMINDER);
    m_screenDirty = true;
}

void DisplayManager::showNotification(const String& title, const String& message, unsigned long durationMs) noexcept {
    if (!m_initialized) return;
    m_cachedTitle = title;
    m_cachedMessage = message;
    m_notificationTimeout = durationMs ? (millis() + durationMs) : 0;
    changeState(DisplayState::NOTIFICATION);
    m_screenDirty = true;
}

void DisplayManager::showError(const String& title, const String& message) noexcept {
    if (!m_initialized) return;
    m_cachedTitle = title;
    m_cachedMessage = message;
    changeState(DisplayState::ERROR);
    m_screenDirty = true;
}

void DisplayManager::showOTAProgress(uint8_t progress) noexcept {
    if (!m_initialized) return;
    if (m_currentState != DisplayState::OTA) {
        changeState(DisplayState::OTA);
    }
    m_cachedOTAProgress = progress;
    m_screenDirty = true;
}

void DisplayManager::showWifiStatus(bool connected, const String& ssid, int32_t signal) noexcept {
    if (!m_initialized) return;
    m_cachedWifiConnected = connected;
    m_cachedSSID = ssid;
    m_cachedSignal = signal;
    changeState(DisplayState::HOME);
    m_screenDirty = true;
}

void DisplayManager::showStorageStatus(const String& storageType, uint32_t usedMB, uint32_t totalMB) noexcept {
    if (!m_initialized) return;
    m_cachedStorageType = storageType;
    m_cachedUsedMB = usedMB;
    m_cachedTotalMB = totalMB;
    changeState(DisplayState::HOME);
    m_screenDirty = true;
}

void DisplayManager::showMessage(const String& title, const String& body, const String& footer) noexcept {
    if (!m_initialized) return;
    m_cachedTitle = title;
    m_cachedMessage = body;
    m_cachedFooter = footer;
    changeState(DisplayState::HOME);
    m_screenDirty = true;
}

void DisplayManager::setBrightness(uint8_t brightness) noexcept {
    if (!m_initialized) return;
    m_brightness = brightness;
    m_display.ssd1306_command(SSD1306_SETCONTRAST);
    m_display.ssd1306_command(brightness);
    LOG_DEBUG("DisplayManager", "Brightness set to %d", brightness);
}

void DisplayManager::sleep() noexcept {
    if (!m_initialized || m_sleeping) return;
    m_sleeping = true;
    m_display.clearDisplay();
    m_display.display();
    m_display.ssd1306_command(SSD1306_DISPLAYOFF);
    LOG_INFO("DisplayManager", "Display sleep");
}

void DisplayManager::wake() noexcept {
    if (!m_initialized || !m_sleeping) return;
    m_sleeping = false;
    m_display.ssd1306_command(SSD1306_DISPLAYON);
    m_screenDirty = true;
    m_lastActivityTime = millis();
    LOG_INFO("DisplayManager", "Display wake");
}

void DisplayManager::displayOn() noexcept {
    if (!m_initialized) return;
    m_display.ssd1306_command(SSD1306_DISPLAYON);
}

void DisplayManager::displayOff() noexcept {
    if (!m_initialized) return;
    m_display.ssd1306_command(SSD1306_DISPLAYOFF);
}

bool DisplayManager::isSleeping() const noexcept {
    return m_sleeping;
}

bool DisplayManager::isAwake() const noexcept {
    return !m_sleeping;
}

void DisplayManager::setRotation(uint8_t rotation) noexcept {
    if (!m_initialized) return;
    m_rotation = rotation & 0x03;
    m_display.setRotation(m_rotation);
    m_screenDirty = true;
    LOG_DEBUG("DisplayManager", "Rotation set to %d", m_rotation);
}

void DisplayManager::setContrast(uint8_t contrast) noexcept {
    if (!m_initialized) return;
    m_contrast = contrast;
    m_display.ssd1306_command(SSD1306_SETCONTRAST);
    m_display.ssd1306_command(contrast);
    LOG_DEBUG("DisplayManager", "Contrast set to %d", contrast);
}

void DisplayManager::setInverted(bool inverted) noexcept {
    if (!m_initialized) return;
    m_inverted = inverted;
    m_display.ssd1306_command(inverted ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
    LOG_DEBUG("DisplayManager", "Inverted set to %s", inverted ? "true" : "false");
}

uint16_t DisplayManager::getWidth() const noexcept {
    return m_displayWidth;
}

uint16_t DisplayManager::getHeight() const noexcept {
    return m_displayHeight;
}

void DisplayManager::changeState(DisplayState newState) noexcept {
    if (m_currentState == newState) return;

    m_previousState = m_currentState;
    m_currentState = newState;
    m_stateStartTime = millis();
    m_animationFrame = 0;
    m_screenDirty = true;
    m_lastActivityTime = millis();
}

void DisplayManager::renderBoot(uint8_t progress) noexcept {
    m_display.clearDisplay();

    drawCenteredText("AURA", 4, 2);
    drawCenteredText(aura::identity::kCodename, 24, 1);
    drawCenteredText(aura::identity::kVersion, 36, 1);

    drawProgressBar(14, 48, 100, 8, progress);

    char pctStr[6];
    snprintf(pctStr, sizeof(pctStr), "%d%%", progress);
    drawCenteredText(pctStr, 58, 1);
}

void DisplayManager::renderHome() noexcept {
    m_display.clearDisplay();

    m_display.setTextSize(3);
    m_display.setTextColor(SSD1306_WHITE);
    const char* aura = "AURA";
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(aura, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 4);
    m_display.print(aura);

    drawStatusIcons();

    if (!m_cachedTitle.isEmpty()) {
        m_display.setTextSize(1);
        m_display.getTextBounds(m_cachedTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 36);
        m_display.print(m_cachedTitle);
    }

    if (!m_cachedMessage.isEmpty()) {
        m_display.setTextSize(1);
        m_display.getTextBounds(m_cachedMessage.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 46);
        m_display.print(m_cachedMessage);
    }

    if (!m_cachedFooter.isEmpty()) {
        m_display.setTextSize(1);
        m_display.getTextBounds(m_cachedFooter.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 56);
        m_display.print(m_cachedFooter);
    }
}

void DisplayManager::renderListening() noexcept {
    m_display.clearDisplay();

    const uint8_t centerX = m_displayWidth / 2;
    const uint8_t centerY = m_displayHeight / 2;

    const uint8_t frame = m_animationFrame % 8;
    const uint8_t radius = 8 + (frame < 4 ? frame : 7 - frame);

    m_display.drawCircle(centerX, centerY, 14, SSD1306_WHITE);
    m_display.drawCircle(centerX, centerY, 13, SSD1306_WHITE);

    m_display.drawLine(centerX, centerY + 14, centerX, centerY + 22, SSD1306_WHITE);
    m_display.drawLine(centerX - 4, centerY + 22, centerX + 4, centerY + 22, SSD1306_WHITE);

    m_display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);

    const char* text = "LISTENING";
    m_display.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 54);
    m_display.print(text);
}

void DisplayManager::renderThinking() noexcept {
    m_display.clearDisplay();

    const char* text = "THINKING";
    m_display.setTextSize(2);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 10);
    m_display.print(text);

    const uint8_t dotY = 40;
    const uint8_t spacing = 18;
    const uint8_t startX = (m_displayWidth - (3 * spacing)) / 2;

    for (uint8_t i = 0; i < 3; ++i) {
        const uint8_t frame = (m_animationFrame + i * 3) % 12;
        const uint8_t radius = 3 + (frame < 6 ? frame : 11 - frame) / 2;
        const uint8_t x = startX + i * spacing + spacing / 2;
        m_display.fillCircle(x, dotY, radius, SSD1306_WHITE);
    }
}

void DisplayManager::renderSpeaking() noexcept {
    m_display.clearDisplay();

    const char* text = "SPEAKING";
    m_display.setTextSize(2);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 8);
    m_display.print(text);

    const uint8_t barCount = 12;
    const uint8_t barWidth = 4;
    const uint8_t spacing = 2;
    const uint8_t startX = (m_displayWidth - (barCount * (barWidth + spacing))) / 2;
    const uint8_t baseY = 30;
    const uint8_t maxHeight = 24;

    for (uint8_t i = 0; i < barCount; ++i) {
        const uint8_t frame = (m_animationFrame + i * 2) % 16;
        const uint8_t height = 4 + (frame < 8 ? frame : 15 - frame) * maxHeight / 8;
        const uint8_t x = startX + i * (barWidth + spacing);
        const uint8_t y = baseY + maxHeight - height;
        m_display.fillRect(x, y, barWidth, height, SSD1306_WHITE);
    }

    m_display.drawLine(startX - 2, baseY + maxHeight + 2,
                       startX + barCount * (barWidth + spacing), baseY + maxHeight + 2,
                       SSD1306_WHITE);
}

void DisplayManager::renderReminder() noexcept {
    m_display.clearDisplay();

    const uint8_t centerX = m_displayWidth / 2;
    const uint8_t bellY = 16;
    const uint8_t bellRadius = 10;

    m_display.drawCircle(centerX, bellY, bellRadius, SSD1306_WHITE);
    m_display.drawLine(centerX - 6, bellY, centerX + 6, bellY, SSD1306_WHITE);
    m_display.drawLine(centerX, bellY - 4, centerX, bellY - 8, SSD1306_WHITE);
    m_display.fillCircle(centerX, bellY - 8, 2, SSD1306_WHITE);

    if (!m_cachedTitle.isEmpty()) {
        m_display.setTextSize(1);
        m_display.setTextColor(SSD1306_WHITE);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 32);
        m_display.print(m_cachedTitle);
    }

    if (!m_cachedMessage.isEmpty()) {
        m_display.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedMessage.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 44);
        m_display.print(m_cachedMessage);
    }
}

void DisplayManager::renderNotification() noexcept {
    m_display.clearDisplay();

    const uint8_t boxX = 4;
    const uint8_t boxY = 4;
    const uint8_t boxW = m_displayWidth - 8;
    const uint8_t boxH = m_displayHeight - 8;

    m_display.drawRect(boxX, boxY, boxW, boxH, SSD1306_WHITE);
    m_display.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, SSD1306_WHITE);

    if (!m_cachedTitle.isEmpty()) {
        m_display.setTextSize(1);
        m_display.setTextColor(SSD1306_WHITE);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor(boxX + 4, boxY + 4);
        m_display.print(m_cachedTitle);
    }

    if (!m_cachedMessage.isEmpty()) {
        m_display.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedMessage.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor(boxX + 4, boxY + 16);
        m_display.print(m_cachedMessage);
    }

    if (m_notificationTimeout && millis() >= m_notificationTimeout) {
        changeState(DisplayState::HOME);
    }
}

void DisplayManager::renderError() noexcept {
    m_display.clearDisplay();

    drawCenteredText("AURA", 4, 2);

    const uint8_t centerX = m_displayWidth / 2;
    const uint8_t centerY = 24;
    const uint8_t size = 12;

    m_display.drawLine(centerX - size, centerY - size, centerX + size, centerY + size, SSD1306_WHITE);
    m_display.drawLine(centerX - size, centerY + size, centerX + size, centerY - size, SSD1306_WHITE);

    if (!m_cachedTitle.isEmpty()) {
        drawCenteredText(m_cachedTitle, 44, 1);
    }

    if (!m_cachedMessage.isEmpty()) {
        drawCenteredText(m_cachedMessage, 54, 1);
    }
}

void DisplayManager::renderOTAProgress() noexcept {
    m_display.clearDisplay();

    const char* title = "OTA UPDATE";
    m_display.setTextSize(2);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 8);
    m_display.print(title);

    drawProgressBar(14, 34, 100, 12, m_cachedOTAProgress);

    char pctStr[6];
    snprintf(pctStr, sizeof(pctStr), "%d%%", m_cachedOTAProgress);
    m_display.setTextSize(2);
    m_display.getTextBounds(pctStr, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 50);
    m_display.print(pctStr);
}

void DisplayManager::renderWifiStatus() noexcept {
    m_display.clearDisplay();

    const char* title = m_cachedWifiConnected ? "WiFi Connected" : "WiFi Disconnected";
    m_display.setTextSize(1);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 4);
    m_display.print(title);

    if (m_cachedWifiConnected && !m_cachedSSID.isEmpty()) {
        m_display.setTextSize(1);
        m_display.getTextBounds(m_cachedSSID.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 18);
        m_display.print(m_cachedSSID);
    }

    drawWifiIcon(54, 32, m_cachedSignal);

    if (m_cachedSignal) {
        char signalStr[16];
        snprintf(signalStr, sizeof(signalStr), "%ld dBm", m_cachedSignal);
        m_display.setTextSize(1);
        m_display.getTextBounds(signalStr, 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 56);
        m_display.print(signalStr);
    }
}

void DisplayManager::renderStorageStatus() noexcept {
    m_display.clearDisplay();

    const char* title = "Storage";
    m_display.setTextSize(2);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, 4);
    m_display.print(title);

    if (!m_cachedStorageType.isEmpty()) {
        m_display.setTextSize(1);
        m_display.getTextBounds(m_cachedStorageType.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 24);
        m_display.print(m_cachedStorageType);
    }

    drawStorageIcon(54, 36);

    if (m_cachedTotalMB > 0) {
        const uint8_t usedPercent = (m_cachedUsedMB * 100) / m_cachedTotalMB;
        drawProgressBar(14, 48, 100, 8, usedPercent);

        char usageStr[24];
        snprintf(usageStr, sizeof(usageStr), "%lu / %lu MB", m_cachedUsedMB, m_cachedTotalMB);
        m_display.setTextSize(1);
        m_display.getTextBounds(usageStr, 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 58);
        m_display.print(usageStr);
    }
}

void DisplayManager::renderMessage() noexcept {
    m_display.clearDisplay();

    if (!m_cachedTitle.isEmpty()) {
        m_display.setTextSize(2);
        m_display.setTextColor(SSD1306_WHITE);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 4);
        m_display.print(m_cachedTitle);
    }

    if (!m_cachedMessage.isEmpty()) {
        m_display.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedMessage.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 28);
        m_display.print(m_cachedMessage);
    }

    if (!m_cachedFooter.isEmpty()) {
        m_display.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        m_display.getTextBounds(m_cachedFooter.c_str(), 0, 0, &x1, &y1, &w, &h);
        m_display.setCursor((m_displayWidth - w) / 2, 52);
        m_display.print(m_cachedFooter);
    }
}

void DisplayManager::renderSleep() noexcept {
    m_display.clearDisplay();
}

void DisplayManager::drawCenteredText(const String& text, uint8_t y, uint8_t textSize) noexcept {
    m_display.setTextSize(textSize);
    m_display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    m_display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    m_display.setCursor((m_displayWidth - w) / 2, y);
    m_display.print(text);
}

void DisplayManager::drawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t percent) noexcept {
    if (percent > 100) percent = 100;

    m_display.drawRect(x, y, width, height, SSD1306_WHITE);

    if (percent > 0) {
        const uint8_t innerWidth = (width - 2) * percent / 100;
        if (innerWidth > 0) {
            m_display.fillRect(x + 1, y + 1, innerWidth, height - 2, SSD1306_WHITE);
        }
    }
}

void DisplayManager::drawWifiIcon(uint8_t x, uint8_t y, int32_t signal) noexcept {
    uint8_t bars = 0;
    if (signal <= -90) bars = 1;
    else if (signal <= -70) bars = 2;
    else if (signal <= -50) bars = 3;
    else if (signal <= 0) bars = 4;
    else bars = 0;

    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t barHeight = 4 + i * 4;
        const uint8_t barY = y + 16 - barHeight;
        if (i < bars) {
            m_display.fillRect(x + i * 4, barY, 3, barHeight, SSD1306_WHITE);
        } else {
            m_display.drawRect(x + i * 4, barY, 3, barHeight, SSD1306_WHITE);
        }
    }

    m_display.drawLine(x + 1, y + 16, x + 14, y + 16, SSD1306_WHITE);
}

void DisplayManager::drawStorageIcon(uint8_t x, uint8_t y) noexcept {
    m_display.drawRect(x, y, 20, 14, SSD1306_WHITE);
    m_display.drawRect(x + 2, y - 4, 16, 6, SSD1306_WHITE);
    m_display.drawLine(x + 2, y, x + 18, y, SSD1306_WHITE);
    m_display.drawLine(x + 6, y + 4, x + 6, y + 10, SSD1306_WHITE);
    m_display.drawLine(x + 12, y + 4, x + 12, y + 10, SSD1306_WHITE);
}

void DisplayManager::drawStatusIcons() noexcept {
    drawWifiIcon(4, 4, m_cachedSignal);
    drawStorageIcon(100, 4);
}

void DisplayManager::updateAnimation() noexcept {
    const unsigned long now = millis();
    if (now - m_lastUpdateTime >= m_animationIntervalMs) {
        m_animationFrame++;
        m_lastUpdateTime = now;

        switch (m_currentState) {
            case DisplayState::LISTENING:
            case DisplayState::THINKING:
            case DisplayState::SPEAKING:
                m_screenDirty = true;
                break;
            default:
                break;
        }
    }
}

void DisplayManager::updateScreenTimeout() noexcept {
    if (m_sleeping) return;

    const unsigned long now = millis();
    if (now - m_lastActivityTime >= m_screenTimeoutMs) {
        sleep();
    }
}