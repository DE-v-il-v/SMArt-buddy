#ifndef AURA_UTILITIES_H
#define AURA_UTILITIES_H

#include <Arduino.h>

#include "config.h"

/**
 * @file utilities.h
 * @brief Lightweight, reusable utility declarations for AURA firmware modules.
 */

/**
 * @brief Determines whether a C-string is null or empty.
 * @param text Pointer to the text to inspect.
 * @return True when @p text is null or contains no characters.
 */
bool isNullOrEmpty(const char* text);

/**
 * @brief Removes leading and trailing whitespace from a string.
 * @param text Source string.
 * @return Trimmed string.
 */
String trimString(const String& text);

/**
 * @brief Converts all characters in a string to lowercase.
 * @param text Source string.
 * @return Lowercase string.
 */
String toLowerCase(const String& text);

/**
 * @brief Converts all characters in a string to uppercase.
 * @param text Source string.
 * @return Uppercase string.
 */
String toUpperCase(const String& text);

/**
 * @brief Formats a byte count using a human-readable unit.
 * @param bytes Number of bytes.
 * @return Formatted byte-count string.
 */
String formatBytes(size_t bytes);

/**
 * @brief Formats a duration in milliseconds as a human-readable time string.
 * @param milliseconds Duration in milliseconds.
 * @return Formatted duration string.
 */
String formatTime(uint32_t milliseconds);

/**
 * @brief Converts milliseconds to whole seconds.
 * @param milliseconds Duration in milliseconds.
 * @return Duration in whole seconds.
 */
uint32_t millisecondsToSeconds(uint32_t milliseconds);

/**
 * @brief Converts seconds to milliseconds.
 * @param seconds Duration in seconds.
 * @return Duration in milliseconds.
 */
uint32_t secondsToMilliseconds(uint32_t seconds);

/**
 * @brief Restricts a value to an inclusive range.
 * @tparam T Comparable value type.
 * @param value Value to constrain.
 * @param minimum Inclusive lower bound.
 * @param maximum Inclusive upper bound.
 * @return The constrained value.
 */
template<typename T>
constexpr T clampValue(T value, T minimum, T maximum)
{
    return (value < minimum) ? minimum :
           (value > maximum) ? maximum :
           value;
}

#endif  // AURA_UTILITIES_H