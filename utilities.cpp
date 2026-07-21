#include "utilities.h"
#include <cstdint>
#include <cstdio>


namespace {

constexpr size_t kBytesPerKilobyte = 1024U;
constexpr uint32_t kMillisecondsPerSecond = 1000U;
constexpr uint32_t kSecondsPerMinute = 60U;
constexpr uint32_t kMinutesPerHour = 60U;

}  // namespace

bool isNullOrEmpty(const char* text)
{
    return text == nullptr || text[0] == '\0';
}

String trimString(const String& text)
{
    String result(text);
    result.trim();
    return result;
}

String toLowerCase(const String& text)
{
    String result(text);
    result.toLowerCase();
    return result;
}

String toUpperCase(const String& text)
{
    String result(text);
    result.toUpperCase();
    return result;
}

String formatBytes(size_t bytes)
{
    constexpr const char* units[] = {"B", "KB", "MB", "GB"};
    constexpr size_t unitCount = sizeof(units) / sizeof(units[0]);

    double value = static_cast<double>(bytes);
    size_t unitIndex = 0U;

    while (value >= static_cast<double>(kBytesPerKilobyte) &&
           unitIndex < (unitCount - 1U)) {
        value /= static_cast<double>(kBytesPerKilobyte);
        ++unitIndex;
    }

    char buffer[24] = {};

    if (unitIndex == 0U) {
        snprintf(buffer, sizeof(buffer), "%zu %s", bytes, units[unitIndex]);
    } else {
        snprintf(
            buffer,
            sizeof(buffer),
            "%.2f %s",
            value,
            units[unitIndex]);
    }

    return String(buffer);
}

String formatTime(uint32_t milliseconds)
{
    const uint32_t totalSeconds = millisecondsToSeconds(milliseconds);
    const uint32_t seconds = totalSeconds % kSecondsPerMinute;
    const uint32_t totalMinutes = totalSeconds / kSecondsPerMinute;
    const uint32_t minutes = totalMinutes % kMinutesPerHour;
    const uint32_t hours = totalMinutes / kMinutesPerHour;

    char buffer[16] = {};

    snprintf(
        buffer,
        sizeof(buffer),
        "%02lu:%02lu:%02lu",
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds));

    return String(buffer);
}

uint32_t millisecondsToSeconds(uint32_t milliseconds)
{
    return milliseconds / kMillisecondsPerSecond;
}

uint32_t secondsToMilliseconds(uint32_t seconds)
{
    constexpr uint32_t maximumSeconds =
        UINT32_MAX / kMillisecondsPerSecond;

    if (seconds > maximumSeconds) {
        return UINT32_MAX;
    }

    return seconds * kMillisecondsPerSecond;
}