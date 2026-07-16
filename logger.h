#pragma once

#ifndef AURA_LOGGER_H
#define AURA_LOGGER_H

#include <Arduino.h>
#include <cstddef>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"

/**
 * @file logger.h
 * @brief Thread-safe logging interface for the AURA AI Desktop Assistant.
 *
 * This class provides centralized logging services for all firmware modules.
 * It supports multiple log levels and is designed for use with FreeRTOS.
 *
 * Features:
 * - Thread-safe serial logging
 * - printf-style formatting
 * - Lightweight implementation
 * - Future support for SD card and OLED logging
 */
class Logger final
{
public:
    Logger() = delete;
    ~Logger() = delete;

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;

    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Initializes the logging subsystem.
     *
     * Creates the internal mutex and starts the Serial interface.
     *
     * @return true if initialization succeeds.
     * @return false otherwise.
     */
    static bool initialize();

    /**
     * @brief Logs a formatted message.
     *
     * @param level Log severity.
     * @param category Module name.
     * @param format printf-style format string.
     */
    static void log(
        LogLevel level,
        const char* category,
        const char* format,
        ...) __attribute__((format(printf, 3, 4)));

    static void debug(
        const char* category,
        const char* format,
        ...) __attribute__((format(printf, 2, 3)));

    static void info(
        const char* category,
        const char* format,
        ...) __attribute__((format(printf, 2, 3)));

    static void warning(
        const char* category,
        const char* format,
        ...) __attribute__((format(printf, 2, 3)));

    static void error(
        const char* category,
        const char* format,
        ...) __attribute__((format(printf, 2, 3)));

private:
    /**
     * @brief Converts a LogLevel into a readable string.
     */
    static const char* levelToString(LogLevel level);

    /**
     * @brief Mutex protecting shared logger resources.
     */
    static SemaphoreHandle_t xLogMutex;

    /**
     * @brief Size of the internal formatting buffer.
     */
    static constexpr std::size_t LOG_BUFFER_SIZE = 256;

    /**
     * @brief Serial baud rate.
     */
    static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
};

#endif // AURA_LOGGER_H