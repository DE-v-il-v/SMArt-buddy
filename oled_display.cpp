#include "oled_display.h"

#include <cmath>
#include <cstdio>

namespace {

constexpr const char* kLogCategory = "OledDisplay";

constexpr uint8_t kColorOn = SSD1306_WHITE;
constexpr uint8_t kColorOff = SSD1306_BLACK;

constexpr int16_t kMargin = 4;
constexpr int16_t kCenterX = OLED_WIDTH / 2;
constexpr int16_t kCenterY = OLED_HEIGHT / 2;

constexpr uint32_t kFrameIntervalMs = 60U;
constexpr uint32_t kBootSegmentIntervalMs = 120U;
constexpr uint32_t kBootDotDelayMs = 600U;
constexpr uint32_t kBootWordmarkDelayMs = 800U;
constexpr uint32_t kBootPulsePeriodMs = 1200U;
constexpr uint32_t kReadyBreathPeriodMs = 4000U;
constexpr uint32_t kListeningPulsePeriodMs = 1200U;
constexpr uint32_t kListeningWaveIntervalMs = 80U;
constexpr uint32_t kThinkingOrbitPeriodMs = 1800U;
constexpr uint32_t kThinkingNotchPeriodMs = 3000U;
constexpr uint32_t kSpeakingBarIntervalMs = 60U;
constexpr uint32_t kWifiArcPeriodMs = 250U;
constexpr uint32_t kWifiMarqueePeriodMs = 900U;
constexpr uint32_t kReminderTransitionMs = 220U;
constexpr uint32_t kReminderClapperPeriodMs = 1500U;
constexpr uint32_t kOtaArrowPeriodMs = 700U;
constexpr uint32_t kErrorShakeDurationMs = 350U;
constexpr uint32_t kErrorBlinkPeriodMs = 1800U;
constexpr uint32_t kErrorBlinkOnMs = 1500U;
constexpr uint32_t kSleepBreathPeriodMs = 6000U;

constexpr int16_t kBootOrbY = 16;
constexpr int16_t kReadyOrbY = 24;
constexpr int16_t kThinkingOrbY = 20;
constexpr int16_t kStandardOrbRadius = 11;
constexpr int16_t kBootOrbRadius = 10;

constexpr int16_t kStandardProgressX = 24;
constexpr int16_t kStandardProgressWidth = 80;
constexpr int16_t kOtaProgressX = 16;
constexpr int16_t kOtaProgressWidth = 96;

constexpr size_t kTitleBufferSize = 32U;
constexpr size_t kBodyBufferSize = 96U;

enum class DisplayScreen : uint8_t {
    Boot,
    Ready,
    Listening,
    Thinking,
    Speaking,
    WifiConnecting,
    WifiConnected,
    Reminder,
    Ota,
    Error,
    Sleep,
    Message,
    CenteredText
};

struct DisplayState {
    DisplayScreen screen = DisplayScreen::Boot;
    uint32_t screenStartedAt = 0U;
    uint32_t lastFrameAt = 0U;
    uint8_t otaPercent = 0U;
    bool initialized = false;
    bool dirty = true;
    char title[kTitleBufferSize] = {};
    char body[kBodyBufferSize] = {};
};

DisplayState gDisplayState;

}  // namespace

OledDisplay::OledDisplay()
    : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1)
{
}

bool OledDisplay::initialize()
{
    if (gDisplayState.initialized) {
        return true;
    }

    Logger::info(kLogCategory, "Initializing SSD1306 display");
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false)) {
        Logger::error(
            kLogCategory,
            "SSD1306 initialization failed at address 0x%02X",
            OLED_ADDRESS);
        return false;
    }

    display.clearDisplay();
    display.setTextColor(kColorOn);
    display.setTextSize(1U);
    display.setTextWrap(false);
    display.display();

    gDisplayState.initialized = true;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.lastFrameAt = 0U;
    gDisplayState.dirty = true;

    Logger::info(kLogCategory, "SSD1306 display initialized");
    return true;
}

void OledDisplay::run()
{
    if (!gDisplayState.initialized) {
        return;
    }

    const uint32_t now = millis();
    if (!gDisplayState.dirty &&
        (now - gDisplayState.lastFrameAt) < kFrameIntervalMs) {
        return;
    }

    const uint32_t elapsed = now - gDisplayState.screenStartedAt;

    const auto sineOffset = [](const uint32_t time,
                               const uint32_t period,
                               const int16_t amplitude) -> int16_t {
        const float phase =
            (static_cast<float>(time % period) / static_cast<float>(period)) *
            2.0F * PI;
        return static_cast<int16_t>(
            std::round(std::sin(phase) * static_cast<float>(amplitude)));
    };

    const auto drawOrb = [this](
                             const int16_t centerX,
                             const int16_t centerY,
                             const int16_t radius,
                             const bool drawCenterDot) {
        display.drawCircle(centerX, centerY, radius, kColorOn);

        if (drawCenterDot) {
            display.fillCircle(centerX - 2, centerY - 2, 2, kColorOn);
        }
    };

    const auto drawWifiIcon = [this](
                                  const int16_t centerX,
                                  const int16_t centerY,
                                  const uint8_t activeArcs) {
        constexpr int16_t kArcRadii[] = {12, 8, 4};

        for (uint8_t index = 0U; index < 3U; ++index) {
            if (index >= activeArcs) {
                continue;
            }

            const int16_t radius = kArcRadii[index];
            const int16_t arcY = centerY - radius + 4;

            display.drawLine(
                centerX - radius,
                arcY,
                centerX - radius + 3,
                arcY + 3,
                kColorOn);
            display.drawLine(
                centerX + radius,
                arcY,
                centerX + radius - 3,
                arcY + 3,
                kColorOn);
            display.drawLine(
                centerX - radius + 3,
                arcY + 3,
                centerX - 3,
                centerY - 2,
                kColorOn);
            display.drawLine(
                centerX + radius - 3,
                arcY + 3,
                centerX + 3,
                centerY - 2,
                kColorOn);
        }

        display.fillCircle(centerX, centerY + 3, 2, kColorOn);
    };

    const auto drawMicIcon = [this](
                                 const int16_t centerX,
                                 const int16_t centerY,
                                 const int16_t scale) {
        constexpr int16_t kBaseBodyWidth = 8;
        constexpr int16_t kBaseBodyHeight = 15;
        constexpr int16_t kArcOffset = 7;
        constexpr int16_t kStandBottomOffset = 14;

        const int16_t bodyWidth = kBaseBodyWidth + scale;
        const int16_t bodyHeight = kBaseBodyHeight + scale;
        const int16_t bodyX = centerX - (bodyWidth / 2);
        const int16_t bodyY = centerY - (bodyHeight / 2);

        display.drawRoundRect(
            bodyX,
            bodyY,
            bodyWidth,
            bodyHeight,
            bodyWidth / 2,
            kColorOn);
        display.drawLine(
            centerX - kArcOffset,
            centerY + 3,
            centerX - kArcOffset,
            centerY + 7,
            kColorOn);
        display.drawLine(
            centerX + kArcOffset,
            centerY + 3,
            centerX + kArcOffset,
            centerY + 7,
            kColorOn);
        display.drawLine(
            centerX - kArcOffset,
            centerY + 7,
            centerX - 4,
            centerY + 10,
            kColorOn);
        display.drawLine(
            centerX + kArcOffset,
            centerY + 7,
            centerX + 4,
            centerY + 10,
            kColorOn);
        display.drawLine(
            centerX - 4,
            centerY + 10,
            centerX + 4,
            centerY + 10,
            kColorOn);
        display.drawLine(
            centerX,
            centerY + 10,
            centerX,
            centerY + kStandBottomOffset,
            kColorOn);
        display.drawLine(
            centerX - 5,
            centerY + kStandBottomOffset,
            centerX + 5,
            centerY + kStandBottomOffset,
            kColorOn);
    };

    const auto drawBell = [this](
                              const int16_t x,
                              const int16_t y,
                              const bool showClapper) {
        display.drawRoundRect(x + 3, y + 2, 8, 9, 4, kColorOn);
        display.drawLine(x + 2, y + 11, x + 12, y + 11, kColorOn);

        if (showClapper) {
            display.fillCircle(x + 7, y + 13, 1, kColorOn);
        }
    };

    const auto drawWarning = [this](
                                 const int16_t centerX,
                                 const int16_t topY,
                                 const int16_t horizontalOffset,
                                 const bool showMark) {
        const int16_t x = centerX + horizontalOffset;

        display.drawLine(x, topY, x - 10, topY + 16, kColorOn);
        display.drawLine(x - 10, topY + 16, x + 10, topY + 16, kColorOn);
        display.drawLine(x + 10, topY + 16, x, topY, kColorOn);
        display.drawLine(x - 1, topY + 6, x + 1, topY + 6, kColorOn);
        display.drawLine(x - 1, topY + 6, x - 1, topY + 11, kColorOn);
        display.drawLine(x + 1, topY + 6, x + 1, topY + 11, kColorOn);

        if (showMark) {
            display.fillCircle(x, topY + 13, 1, kColorOn);
        }
    };

    const auto drawDownloadIcon = [this](
                                      const int16_t centerX,
                                      const int16_t topY,
                                      const int16_t arrowOffset) {
        const int16_t arrowY = topY + arrowOffset;

        display.drawRoundRect(centerX - 9, topY, 18, 15, 3, kColorOn);
        display.drawLine(centerX, arrowY + 2, centerX, arrowY + 8, kColorOn);
        display.drawLine(
            centerX - 3,
            arrowY + 6,
            centerX,
            arrowY + 9,
            kColorOn);
        display.drawLine(
            centerX + 3,
            arrowY + 6,
            centerX,
            arrowY + 9,
            kColorOn);
        display.drawLine(
            centerX - 5,
            topY + 11,
            centerX - 5,
            topY + 13,
            kColorOn);
        display.drawLine(
            centerX - 5,
            topY + 13,
            centerX + 5,
            topY + 13,
            kColorOn);
        display.drawLine(
            centerX + 5,
            topY + 13,
            centerX + 5,
            topY + 11,
            kColorOn);
    };

    const auto drawSignalBars = [this](const int16_t x, const int16_t y) {
        constexpr int16_t kBarCount = 4;
        constexpr int16_t kBarSpacing = 4;

        for (int16_t index = 0; index < kBarCount; ++index) {
            const int16_t height = 2 + (index * 2);
            display.fillRoundRect(
                x + (index * kBarSpacing),
                y - height,
                2,
                height,
                1,
                kColorOn);
        }
    };

    display.clearDisplay();
    display.setTextColor(kColorOn);
    display.setTextSize(1U);
    display.setTextWrap(false);

    switch (gDisplayState.screen) {
        case DisplayScreen::Boot: {
            constexpr uint8_t kBootSegmentCount = 10U;
            const uint8_t completedSegments = static_cast<uint8_t>(
                (elapsed / kBootSegmentIntervalMs) > kBootSegmentCount
                    ? kBootSegmentCount
                    : (elapsed / kBootSegmentIntervalMs));
            const int16_t radius =
                kBootOrbRadius +
                ((elapsed > kBootPulsePeriodMs)
                     ? sineOffset(elapsed, kBootPulsePeriodMs, 1)
                     : 0);

            drawOrb(
                kCenterX,
                kBootOrbY,
                radius,
                elapsed >= kBootDotDelayMs);

            if (elapsed >= kBootWordmarkDelayMs) {
                drawCenteredString("A U R A", 30, 1U);
            }

            drawProgressBar(
                kStandardProgressX,
                46,
                kStandardProgressWidth,
                3,
                static_cast<uint8_t>(completedSegments * 10U));
            drawCenteredString(
                String("v") + AURA_VERSION + "  INITIALIZING",
                54,
                1U);
            break;
        }

        case DisplayScreen::Ready: {
            const int16_t radius =
                kStandardOrbRadius +
                ((sineOffset(elapsed, kReadyBreathPeriodMs, 1) > 0) ? 1 : 0);

            drawWifiIcon(10, 7, 3U);
            display.setCursor(OLED_WIDTH - 29, 2);
            display.print("READY");
            drawMicIcon(OLED_WIDTH - 9, OLED_HEIGHT - 8, 0);

            drawOrb(kCenterX, kReadyOrbY, radius, true);

            if (((elapsed / kWifiMarqueePeriodMs) % 9U) == 0U) {
                const float phase =
                    (static_cast<float>(elapsed % kWifiMarqueePeriodMs) /
                     static_cast<float>(kWifiMarqueePeriodMs)) *
                    2.0F * PI;
                const int16_t sparkX =
                    kCenterX +
                    static_cast<int16_t>(
                        std::round(std::cos(phase) * static_cast<float>(radius)));
                const int16_t sparkY =
                    kReadyOrbY +
                    static_cast<int16_t>(
                        std::round(std::sin(phase) * static_cast<float>(radius)));
                display.drawPixel(sparkX, sparkY, kColorOn);
            }

            drawCenteredString("Ready", 39, 1U);
            break;
        }

        case DisplayScreen::Listening: {
            const int16_t scale =
                (sineOffset(elapsed, kListeningPulsePeriodMs, 1) > 0) ? 1 : 0;
            const uint8_t activeRing = static_cast<uint8_t>(
                (elapsed % kListeningPulsePeriodMs) /
                (kListeningPulsePeriodMs / 3U));

            drawMicIcon(kCenterX, 18, scale);

            for (uint8_t ring = 0U; ring < 3U; ++ring) {
                if (ring != activeRing) {
                    continue;
                }

                const int16_t leftX = 43 - static_cast<int16_t>(ring * 5U);
                const int16_t rightX = 85 + static_cast<int16_t>(ring * 5U);

                display.drawLine(leftX, 12, leftX - 3, 16, kColorOn);
                display.drawLine(leftX, 24, leftX - 3, 20, kColorOn);
                display.drawLine(rightX, 12, rightX + 3, 16, kColorOn);
                display.drawLine(rightX, 24, rightX + 3, 20, kColorOn);
            }

            drawCenteredString("Listening...", 35, 1U);

            for (uint8_t index = 0U; index < 10U; ++index) {
                const uint32_t seed =
                    (elapsed / kListeningWaveIntervalMs) + (index * 19U);
                const int16_t height =
                    2 + static_cast<int16_t>((seed * 7U) % 9U);
                const int16_t x = 19 + static_cast<int16_t>(index * 9U);

                display.fillRoundRect(x, 56 - height, 4, height, 2, kColorOn);
            }
            break;
        }

        case DisplayScreen::Thinking: {
            drawOrb(kCenterX, kThinkingOrbY, kStandardOrbRadius, false);

            for (uint8_t index = 0U; index < 3U; ++index) {
                const float phase =
                    ((static_cast<float>(elapsed % kThinkingOrbitPeriodMs) /
                      static_cast<float>(kThinkingOrbitPeriodMs)) *
                     2.0F * PI) +
                    (static_cast<float>(index) * 2.0F * PI / 3.0F);
                const int16_t particleX =
                    kCenterX +
                    static_cast<int16_t>(std::round(std::cos(phase) * 6.0F));
                const int16_t particleY =
                    kThinkingOrbY +
                    static_cast<int16_t>(std::round(std::sin(phase) * 4.0F));

                display.fillCircle(particleX, particleY, 1, kColorOn);
            }

            const float notchPhase =
                -((static_cast<float>(elapsed % kThinkingNotchPeriodMs) /
                   static_cast<float>(kThinkingNotchPeriodMs)) *
                  2.0F * PI);

            display.drawPixel(
                kCenterX +
                    static_cast<int16_t>(
                        std::round(std::cos(notchPhase) * kStandardOrbRadius)),
                kThinkingOrbY +
                    static_cast<int16_t>(
                        std::round(std::sin(notchPhase) * kStandardOrbRadius)),
                kColorOn);

            drawCenteredString("Thinking", 37, 1U);

            const uint8_t activeDot =
                static_cast<uint8_t>((elapsed / 200U) % 3U);
            for (uint8_t index = 0U; index < 3U; ++index) {
                display.fillCircle(
                    58 + static_cast<int16_t>(index * 6U),
                    53,
                    (index == activeDot) ? 2 : 1,
                    kColorOn);
            }
            break;
        }

        case DisplayScreen::Speaking: {
            constexpr uint8_t kBarCount = 9U;
            constexpr int16_t kBarX = 28;
            constexpr int16_t kBarSpacing = 8;

            for (uint8_t index = 0U; index < kBarCount; ++index) {
                const uint32_t seed =
                    (elapsed / kSpeakingBarIntervalMs) + (index * 17U);
                const uint32_t multiplier =
                    (index < 2U || index > 6U) ? 3U : 5U;
                const int16_t height =
                    5 + static_cast<int16_t>((seed * multiplier) % 15U);
                const int16_t x =
                    kBarX + static_cast<int16_t>(index * kBarSpacing);

                display.fillRoundRect(
                    x,
                    25 - (height / 2),
                    4,
                    height,
                    2,
                    kColorOn);
            }

            drawCenteredString("Speaking", 43, 1U);
            break;
        }

        case DisplayScreen::WifiConnecting: {
            const uint8_t activeArcs =
                static_cast<uint8_t>(((elapsed / kWifiArcPeriodMs) % 3U) + 1U);
            const int16_t marqueeOffset = static_cast<int16_t>(
                (elapsed % kWifiMarqueePeriodMs) * 72U / kWifiMarqueePeriodMs);

            drawWifiIcon(kCenterX, 20, activeArcs);
            drawCenteredString("Connecting...", 33, 1U);

            display.drawRoundRect(
                kStandardProgressX,
                46,
                kStandardProgressWidth,
                4,
                2,
                kColorOn);
            display.fillRoundRect(
                kStandardProgressX + 3 + marqueeOffset,
                47,
                8,
                2,
                1,
                kColorOn);
            break;
        }

        case DisplayScreen::WifiConnected:
            drawOrb(kCenterX, 19, kStandardOrbRadius, false);
            display.drawLine(59, 19, 63, 23, kColorOn);
            display.drawLine(63, 23, 70, 15, kColorOn);
            drawCenteredString("Connected", 33, 1U);
            drawSignalBars(42, 54);
            display.setCursor(61, 48);
            display.print("Wi-Fi");
            break;

        case DisplayScreen::Reminder: {
            const uint32_t transitionElapsed =
                elapsed < kReminderTransitionMs ? elapsed : kReminderTransitionMs;
            const int16_t yOffset = static_cast<int16_t>(
                (kReminderTransitionMs - transitionElapsed) * 20U /
                kReminderTransitionMs);
            const bool showClapper =
                ((elapsed / kReminderClapperPeriodMs) % 2U) == 0U;

            drawBell(5, 2 + yOffset, showClapper);
            display.setCursor(24, 5 + yOffset);
            display.print("Reminder");
            display.drawLine(
                kMargin,
                17 + yOffset,
                OLED_WIDTH - kMargin - 1,
                17 + yOffset,
                kColorOn);

            display.setCursor(8, 24 + yOffset);
            display.print(gDisplayState.title);
            display.setCursor(8, 35 + yOffset);
            display.print(gDisplayState.body);

            display.drawRoundRect(18, 52 + yOffset, 40, 9, 2, kColorOn);
            display.fillRoundRect(70, 52 + yOffset, 40, 9, 2, kColorOn);

            display.setTextColor(kColorOff);
            display.setCursor(75, 53 + yOffset);
            display.print("Snooze");

            display.setTextColor(kColorOn);
            display.setCursor(22, 53 + yOffset);
            display.print("Dismiss");
            break;
        }

        case DisplayScreen::Ota: {
            const int16_t arrowOffset = static_cast<int16_t>(
                ((elapsed % kOtaArrowPeriodMs) * 3U) / kOtaArrowPeriodMs);
            char percentText[8] = {};

            snprintf(
                percentText,
                sizeof(percentText),
                "%u%%",
                static_cast<unsigned int>(gDisplayState.otaPercent));

            drawDownloadIcon(kCenterX, 3, arrowOffset);
            drawCenteredString(percentText, 22, 2U);
            drawProgressBar(
                kOtaProgressX,
                38,
                kOtaProgressWidth,
                4,
                gDisplayState.otaPercent);
            drawCenteredString("Updating firmware...", 48, 1U);
            drawCenteredString("Do not power off", 56, 1U);
            display.drawLine(39, OLED_HEIGHT - 1, 89, OLED_HEIGHT - 1, kColorOn);
            break;
        }

        case DisplayScreen::Error: {
            int16_t shakeOffset = 0;

            if (elapsed < kErrorShakeDurationMs) {
                const float decay =
                    1.0F -
                    (static_cast<float>(elapsed) /
                     static_cast<float>(kErrorShakeDurationMs));
                shakeOffset = static_cast<int16_t>(std::round(
                    std::sin(static_cast<float>(elapsed) * 0.072F) *
                    3.0F * decay));
            }

            drawWarning(
                kCenterX,
                4,
                shakeOffset,
                (elapsed % kErrorBlinkPeriodMs) < kErrorBlinkOnMs);
            drawCenteredString(gDisplayState.title, 25, 1U);
            drawCenteredString(gDisplayState.body, 38, 1U);

            display.fillRoundRect(48, 53, 32, 9, 3, kColorOn);
            display.setTextColor(kColorOff);
            display.setCursor(55, 54);
            display.print("Retry");
            display.setTextColor(kColorOn);
            break;
        }

        case DisplayScreen::Sleep: {
            const bool denseDither =
                sineOffset(elapsed, kSleepBreathPeriodMs, 1) >= 0;

            for (int16_t y = 22; y < 31; ++y) {
                for (int16_t x = 56; x < 65; ++x) {
                    const int16_t deltaX = x - 60;
                    const int16_t deltaY = y - 26;
                    const bool outerCircle =
                        (deltaX * deltaX) + (deltaY * deltaY) <= 16;
                    const bool innerCircle =
                        ((deltaX - 2) * (deltaX - 2)) +
                        ((deltaY + 1) * (deltaY + 1)) <= 16;
                    const int16_t ditherPeriod = denseDither ? 3 : 4;

                    if (outerCircle && !innerCircle &&
                        ((x + y) % ditherPeriod) == 0) {
                        display.drawPixel(x, y, kColorOn);
                    }
                }
            }

            constexpr const char* kSleepTime = "--:--";
            for (uint8_t index = 0U; kSleepTime[index] != '\0'; ++index) {
                if (((index + (elapsed / 1000U)) % 3U) != 0U) {
                    display.setCursor(53 + static_cast<int16_t>(index * 6U), 37);
                    display.print(kSleepTime[index]);
                }
            }
            break;
        }

        case DisplayScreen::Message:
            drawHeader(gDisplayState.title);
            drawCenteredString(gDisplayState.body, 29, 1U);
            drawFooter("AURA");
            break;

        case DisplayScreen::CenteredText:
            drawCenteredString(gDisplayState.body, 28, 1U);
            break;
    }

    update();
    gDisplayState.lastFrameAt = now;
    gDisplayState.dirty = false;
}

void OledDisplay::clear()
{
    if (!gDisplayState.initialized) {
        return;
    }

    display.clearDisplay();
    gDisplayState.dirty = true;
}

void OledDisplay::update()
{
    if (gDisplayState.initialized) {
        display.display();
    }
}

void OledDisplay::setBrightness(const uint8_t brightness)
{
    if (!gDisplayState.initialized) {
        Logger::warning(
            kLogCategory,
            "Brightness requested before display initialization");
        return;
    }

    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(brightness);

    Logger::info(
        kLogCategory,
        "OLED brightness set to %u",
        static_cast<unsigned int>(brightness));
}

void OledDisplay::showBootScreen()
{
    gDisplayState.screen = DisplayScreen::Boot;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing boot screen");
}

void OledDisplay::showReadyScreen()
{
    gDisplayState.screen = DisplayScreen::Ready;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing ready screen");
}

void OledDisplay::showListeningScreen()
{
    gDisplayState.screen = DisplayScreen::Listening;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing listening screen");
}

void OledDisplay::showThinkingScreen()
{
    gDisplayState.screen = DisplayScreen::Thinking;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing thinking screen");
}

void OledDisplay::showSpeakingScreen()
{
    gDisplayState.screen = DisplayScreen::Speaking;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing speaking screen");
}

void OledDisplay::showWifiConnecting()
{
    gDisplayState.screen = DisplayScreen::WifiConnecting;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing Wi-Fi connecting screen");
}

void OledDisplay::showWifiConnected()
{
    gDisplayState.screen = DisplayScreen::WifiConnected;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
    Logger::info(kLogCategory, "Showing Wi-Fi connected screen");
}

void OledDisplay::showReminder(const String& reminder)
{
    snprintf(
        gDisplayState.title,
        sizeof(gDisplayState.title),
        "%s",
        reminder.c_str());
    snprintf(
        gDisplayState.body,
        sizeof(gDisplayState.body),
        "%s",
        "Say dismiss or snooze");

    gDisplayState.screen = DisplayScreen::Reminder;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;

    Logger::info(kLogCategory, "Showing reminder");
}

void OledDisplay::showOTAUpdate(const uint8_t percent)
{
    gDisplayState.otaPercent = percent > 100U ? 100U : percent;
    gDisplayState.screen = DisplayScreen::Ota;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
}

void OledDisplay::showError(const String& message)
{
    snprintf(
        gDisplayState.title,
        sizeof(gDisplayState.title),
        "%s",
        message.c_str());
    snprintf(
        gDisplayState.body,
        sizeof(gDisplayState.body),
        "%s",
        "Check connection and retry");

    gDisplayState.screen = DisplayScreen::Error;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;

    Logger::error(kLogCategory, "Displaying error: %s", gDisplayState.title);
}

void OledDisplay::showCenteredText(const String& text)
{
    snprintf(
        gDisplayState.body,
        sizeof(gDisplayState.body),
        "%s",
        text.c_str());

    gDisplayState.screen = DisplayScreen::CenteredText;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
}

void OledDisplay::showMessage(const String& title, const String& body)
{
    snprintf(
        gDisplayState.title,
        sizeof(gDisplayState.title),
        "%s",
        title.c_str());
    snprintf(
        gDisplayState.body,
        sizeof(gDisplayState.body),
        "%s",
        body.c_str());

    gDisplayState.screen = DisplayScreen::Message;
    gDisplayState.screenStartedAt = millis();
    gDisplayState.dirty = true;
}

void OledDisplay::showStatus(const AuraState state)
{
    switch (state) {
        case BOOTING:
            showBootScreen();
            break;

        case CONNECTING_WIFI:
            showWifiConnecting();
            break;

        case READY:
            showReadyScreen();
            break;

        case LISTENING:
            showListeningScreen();
            break;

        case PROCESSING:
            showThinkingScreen();
            break;

        case SPEAKING:
            showSpeakingScreen();
            break;

        case MIC_MUTED:
            showMessage("Microphone", "Muted");
            break;

        case SETTINGS_MODE:
            gDisplayState.screen = DisplayScreen::Sleep;
            gDisplayState.screenStartedAt = millis();
            gDisplayState.dirty = true;
            Logger::info(kLogCategory, "Showing sleep screen");
            break;

        case OTA_UPDATE:
            showOTAUpdate(gDisplayState.otaPercent);
            break;

        case ERROR_STATE:
            showError("System error");
            break;

        default:
            Logger::warning(kLogCategory, "Unknown AURA state");
            showReadyScreen();
            break;
    }
}

void OledDisplay::drawCenteredString(
    const String& text,
    const int16_t y,
    const uint8_t textSize)
{
    int16_t boundsX = 0;
    int16_t boundsY = 0;
    uint16_t textWidth = 0U;
    uint16_t textHeight = 0U;

    display.setTextSize(textSize);
    display.getTextBounds(
        text.c_str(),
        0,
        y,
        &boundsX,
        &boundsY,
        &textWidth,
        &textHeight);

    const int16_t x =
        (OLED_WIDTH - static_cast<int16_t>(textWidth)) / 2;

    display.setCursor(x < 0 ? 0 : x, y);
    display.print(text);
    display.setTextSize(1U);
}

void OledDisplay::drawHeader(const String& title)
{
    display.setTextSize(1U);
    display.setCursor(kMargin, kMargin);
    display.print(title);
    display.drawLine(
        kMargin,
        14,
        OLED_WIDTH - kMargin - 1,
        14,
        kColorOn);
}

void OledDisplay::drawFooter(const String& text)
{
    display.setTextSize(1U);
    display.setCursor(kMargin, OLED_HEIGHT - 9);
    display.print(text);
}

void OledDisplay::drawProgressBar(
    const int16_t x,
    const int16_t y,
    const int16_t width,
    const int16_t height,
    const uint8_t percent)
{
    constexpr int16_t kMinimumWidth = 3;
    constexpr int16_t kMinimumHeight = 2;
    constexpr int16_t kSegmentWidth = 7;
    constexpr int16_t kSegmentGap = 1;

    if (width < kMinimumWidth || height < kMinimumHeight) {
        return;
    }

    const uint8_t constrainedPercent = percent > 100U ? 100U : percent;
    const int16_t innerWidth = width - 2;
    const int16_t segmentCount =
        innerWidth / (kSegmentWidth + kSegmentGap);
    const int16_t filledSegments = static_cast<int16_t>(
        (static_cast<uint32_t>(segmentCount) * constrainedPercent + 99U) /
        100U);

    display.drawRoundRect(x, y, width, height, height / 2, kColorOn);

    for (int16_t index = 0; index < filledSegments; ++index) {
        const int16_t segmentX =
            x + 1 + (index * (kSegmentWidth + kSegmentGap));
        const int16_t rightEdge = x + width - 1;

        if (segmentX >= rightEdge) {
            break;
        }

        const int16_t availableWidth = rightEdge - segmentX;
        const int16_t fillWidth =
            availableWidth < kSegmentWidth ? availableWidth : kSegmentWidth;

        display.fillRoundRect(
            segmentX,
            y + 1,
            fillWidth,
            height - 2,
            (height - 2) / 2,
            kColorOn);
    }
}