#include "system_manager.h"
#include <ESPmDNS.h>

/// Global SystemManager instance
SystemManager systemManager;

// ============================================================================
// Anonymous Namespace - Internal Helpers
// ============================================================================

namespace {

constexpr const char* kModuleNames[] = {
    "StorageManager",
    "WiFiManager",
    "DisplayManager",
    "AudioManager",
    "SpeechToText",
    "GeminiClient",
    "TextToSpeech",
    "ConversationManager",
    "ReminderManager",
    "OtaManager",
    "WebPortal"
};

constexpr size_t kModuleCount = sizeof(kModuleNames) / sizeof(kModuleNames[0]);

/**
 * @brief Safe string assignment with reserve
 */
void safeAssign(String& dest, const String& src) noexcept {
    dest = src;
}

}  // namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

SystemManager::SystemManager() noexcept
    : m_initialized(false),
      m_currentState(SystemState::BOOTING),
      m_lastError(SystemError::NONE),
      m_info(),
      m_bootTime(0),
      m_lastHealthCheck(0),
      m_moduleInitStartTime(0),
      m_initModuleIndex(0) {
}

SystemManager::~SystemManager() noexcept {
    if (m_initialized) {
        shutdown();
    }
}

// ============================================================================
// Public API - Lifecycle
// ============================================================================

bool SystemManager::initialize() noexcept {
    if (m_initialized) {
        Logger::warning(kLogCategory, "Already initialized");
        return true;
    }

    Logger::info(kLogCategory, "=== %s ===", aura::identity::kProjectName);
    Logger::info(kLogCategory, "Firmware: %s", aura::identity::kVersion);
    Logger::info(kLogCategory, "Codename: %s", aura::identity::kCodename);
    Logger::info(kLogCategory, "Booting...");

    m_bootTime = millis();
    changeState(SystemState::INITIALIZING);

    // Initialize static system info
    safeAssign(m_info.firmwareVersion, AURA_VERSION);
    safeAssign(m_info.deviceName, AURA_NAME);
    m_info.uptime = 0;
    m_info.wifiConnected = false;
    m_info.otaRunning = false;
    m_info.conversationRunning = false;
    m_info.reminderRunning = false;

    // Initialize all modules
    if (!initializeModules()) {
        Logger::error(kLogCategory, "Module initialization failed");
        rollbackInitialization();
        changeState(SystemState::ERROR);
        setError(SystemError::INIT_FAILED);
        displayManager.showError("SYSTEM ERROR", "See Serial Monitor");
        return false;
    }

    m_initialized = true;
    changeState(SystemState::READY);

    const unsigned long initTimeMs = millis() - m_bootTime;
    Logger::info(kLogCategory, "System initialization complete (%lu ms)", initTimeMs);
    Logger::info(kLogCategory, "Free heap: %u bytes", ESP.getFreeHeap());

    return true;
}

void SystemManager::run() noexcept {
    update();
}

void SystemManager::update() noexcept {
    if (!m_initialized) return;

    unsigned long now = millis();
    m_info.uptime = (now - m_bootTime) / 1000;

    // Update all modules
    updateModules();

    // Health monitoring
    if (now - m_lastHealthCheck >= kHealthCheckIntervalMs) {
        m_lastHealthCheck = now;
        checkHealth();
    }

    // State-specific logic
    switch (m_currentState) {
        case SystemState::LOW_POWER:
            // Keep modules in low power
            break;

        case SystemState::UPDATING:
            if (!otaManager.isBusy()) {
                changeState(SystemState::READY);
            }
            break;

        case SystemState::ERROR:
            // Error state - wait for recovery or restart
            break;

        default:
            break;
    }
}

void SystemManager::shutdown() noexcept {
    Logger::info(kLogCategory, "Shutting down...");

    changeState(SystemState::SHUTDOWN);

    // Stop all modules gracefully
    conversationManager.stopConversation();
    textToSpeech.stop();
    audioManager.stopPlayback();
    audioManager.stopRecording();
    ledRing.turnOff();
    displayManager.sleep();
    webPortal.stop();
    wifiManager.disconnect();
    storageManager.unmountSPIFFS();
    storageManager.unmountSD();

    Logger::info(kLogCategory, "Shutdown complete");
}

void SystemManager::restart() noexcept {
    Logger::warning(kLogCategory, "Restarting device...");
    displayManager.showMessage("Restarting...", "");
    delay(500);  // Allow display to update
    ESP.restart();
}

void SystemManager::factoryReset() noexcept {
    Logger::warning(kLogCategory, "Factory reset initiated");

    // Clear all settings
    storageManager.formatSPIFFS();
    wifiManager.clearCredentials();
    settingsManager.factoryReset();

    // Reset all modules
    conversationManager.clearHistory();
    reminderManager.clearReminders();
    otaManager.cancelUpdate();

    Logger::info(kLogCategory, "Factory reset complete");
    restart();
}

void SystemManager::enterLowPower() noexcept {
    if (m_currentState == SystemState::LOW_POWER) return;

    Logger::info(kLogCategory, "Entering low power mode");

    conversationManager.stopConversation();
    textToSpeech.stop();
    audioManager.stopPlayback();
    audioManager.stopRecording();
    displayManager.sleep();
    ledRing.turnOff();

    // Disable WiFi to save power
    wifiManager.disconnect();

    changeState(SystemState::LOW_POWER);
}

void SystemManager::exitLowPower() noexcept {
    if (m_currentState != SystemState::LOW_POWER) return;

    Logger::info(kLogCategory, "Exiting low power mode");

    // Reconnect WiFi
    wifiManager.reconnect();

    // Wake display
    displayManager.wake();
    displayManager.showHome();

    changeState(SystemState::READY);
}

bool SystemManager::checkHealth() noexcept {
    bool healthy = true;

    // Monitor memory
    monitorMemory();
    if (m_info.freeHeap < kMinimumFreeHeap) {
        Logger::warning(kLogCategory, "Low memory: %u bytes (min: %u)",
            m_info.freeHeap, kMinimumFreeHeap);
        healthy = false;
    }

    // Monitor WiFi
    monitorWiFi();
    if (!wifiManager.isConnected()) {
        Logger::warning(kLogCategory, "WiFi disconnected");
        healthy = false;
    }

    // Monitor OTA
    monitorOTA();

    // Monitor reminders
    monitorReminders();

    // Monitor conversation
    monitorConversation();

    // Monitor tasks
    monitorTasks();

    m_info.wifiConnected = wifiManager.isConnected();
    m_info.otaRunning = otaManager.isBusy();
    m_info.conversationRunning = conversationManager.isBusy();
    m_info.reminderRunning = reminderManager.isBusy();

    if (!healthy) {
        setError(SystemError::UNKNOWN);
    }

    return healthy;
}

const SystemInfo& SystemManager::getSystemInfo() const noexcept {
    return m_info;
}

SystemState SystemManager::getState() const noexcept {
    return m_currentState;
}

SystemError SystemManager::getError() const noexcept {
    return m_lastError;
}

bool SystemManager::isInitialized() const noexcept {
    return m_initialized;
}

bool SystemManager::isBusy() const noexcept {
    return m_currentState != SystemState::READY &&
           m_currentState != SystemState::LOW_POWER;
}

// ============================================================================
// Private Methods
// ============================================================================

void SystemManager::changeState(SystemState newState) noexcept {
    if (m_currentState == newState) return;

    static constexpr bool validTransition[8][8] = {
        // BOOTING, INITIALIZING, READY, BUSY, LOW_POWER, UPDATING, ERROR, SHUTDOWN
        {0, 1, 0, 0, 0, 0, 1, 1},   // BOOTING
        {0, 0, 1, 0, 0, 0, 1, 1},   // INITIALIZING
        {0, 0, 0, 1, 1, 1, 1, 1},   // READY
        {0, 0, 1, 0, 1, 1, 1, 1},   // BUSY
        {0, 0, 1, 0, 0, 0, 1, 1},   // LOW_POWER
        {0, 0, 1, 0, 0, 0, 1, 1},   // UPDATING
        {1, 1, 1, 0, 0, 0, 0, 1},   // ERROR
        {0, 0, 0, 0, 0, 0, 0, 0}    // SHUTDOWN
    };

    if (!validTransition[static_cast<uint8_t>(m_currentState)]
                        [static_cast<uint8_t>(newState)]) {
        Logger::warning(kLogCategory, "Invalid state transition %d -> %d",
            static_cast<int>(m_currentState), static_cast<int>(newState));
        return;
    }

    Logger::debug(kLogCategory, "State: %d -> %d",
        static_cast<int>(m_currentState), static_cast<int>(newState));
    m_currentState = newState;
}

void SystemManager::setError(SystemError error) noexcept {
    if (m_lastError == error) return;
    m_lastError = error;
    Logger::error(kLogCategory, "Error: %d", static_cast<int>(error));
}

bool SystemManager::initializeModules() noexcept {
    m_moduleInitStartTime = millis();

    // 1. StorageManager (must be first - other modules depend on it)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[0]);
    if (!storageManager.initialize()) {
        Logger::error(kLogCategory, "Failed to initialize %s", kModuleNames[0]);
        return false;
    }
    m_initModuleIndex = 1;

    // 2. DisplayManager (early for status feedback)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[2]);
    if (!displayManager.initialize()) {
        Logger::error(kLogCategory, "Failed to initialize %s", kModuleNames[2]);
        return false;
    }
    displayManager.showBoot(10);
    m_initModuleIndex = 2;

    // 3. WiFiManager (needed by network modules)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[1]);
    if (!wifiManager.initialize()) {
        Logger::error(kLogCategory, "Failed to initialize %s", kModuleNames[1]);
        return false;
    }

    // Try to connect with stored credentials
    if (wifiManager.hasCredentials()) {
        Logger::info(kLogCategory, "Connecting to saved WiFi...");
        wifiManager.reconnect();
        displayManager.showBoot(30);
    } else {
        Logger::info(kLogCategory, "No WiFi credentials, starting AP mode");
        wifiManager.startAccessPoint(AP_SSID, AP_PASSWORD);
        displayManager.showBoot(50);
    }
    m_initModuleIndex = 3;

    // 4. AudioManager
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[3]);
    if (!audioManager.initialize()) {
        Logger::error(kLogCategory, "Failed to initialize %s", kModuleNames[3]);
        return false;
    }
    m_initModuleIndex = 4;

    // 5. WebPortal (needs WiFi)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[10]);
    if (!webPortal.initialize()) {
        Logger::error(kLogCategory, "Failed to initialize %s", kModuleNames[10]);
        return false;
    }
    webPortal.start();
    displayManager.showBoot(70);
    m_initModuleIndex = 5;

    // 6. SpeechToText (needs WiFi)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[4]);
    if (!speechToText.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[4]);
        // Non-critical, continue
    }
    m_initModuleIndex = 6;

    // 7. GeminiClient (needs WiFi)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[5]);
    if (!geminiClient.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[5]);
    }
    m_initModuleIndex = 7;

    // 8. TextToSpeech (needs WiFi + Audio)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[6]);
    if (!textToSpeech.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[6]);
    }
    m_initModuleIndex = 8;

    // 9. ConversationManager (needs STT, Gemini, TTS)
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[7]);
    if (!conversationManager.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[7]);
    }
    m_initModuleIndex = 9;

    // 10. ReminderManager
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[8]);
    if (!reminderManager.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[8]);
    }
    m_initModuleIndex = 10;

    // 11. OtaManager
    Logger::info(kLogCategory, "Initializing: %s", kModuleNames[9]);
    if (!otaManager.initialize()) {
        Logger::warning(kLogCategory, "Failed to initialize %s (continuing)", kModuleNames[9]);
    }
    m_initModuleIndex = 11;

    displayManager.showBoot(100);
    delay(100);

    return true;
}

void SystemManager::updateModules() noexcept {
    storageManager.update();
    wifiManager.update();
    displayManager.update();
    audioManager.update();
    speechToText.update();
    geminiClient.update();
    textToSpeech.update();
    conversationManager.update();
    reminderManager.update();
    otaManager.update();
    webPortal.update();
}

void SystemManager::monitorMemory() noexcept {
    m_info.freeHeap = ESP.getFreeHeap();
    m_info.minimumHeap = ESP.getMinFreeHeap();
}

void SystemManager::monitorTasks() noexcept {
    // Check for stuck modules
    if (conversationManager.isBusy()) {
        // Conversation has internal timeouts
    }
    if (reminderManager.isBusy()) {
        // Reminder has internal timeouts
    }
}

void SystemManager::monitorWiFi() noexcept {
    if (!wifiManager.isConnected()) {
        if (wifiManager.getState() == WifiState::DISCONNECTED) {
            wifiManager.reconnect();
        }
    }
}

void SystemManager::monitorOTA() noexcept {
    if (otaManager.isBusy()) {
        m_info.otaRunning = true;
        if (otaManager.getState() == OTAState::DOWNLOADING) {
            displayManager.showOTAProgress(otaManager.getProgress());
        }
    } else {
        m_info.otaRunning = false;
    }
}

void SystemManager::monitorReminders() noexcept {
    m_info.reminderRunning = reminderManager.isBusy();
}

void SystemManager::monitorConversation() noexcept {
    m_info.conversationRunning = conversationManager.isBusy();
}

void SystemManager::rollbackInitialization() noexcept {
    Logger::warning(kLogCategory, "Rolling back initialization...");

    // Reverse order cleanup - call proper shutdown methods, not destructors
    if (m_initModuleIndex >= 11) otaManager.cancelUpdate();
    if (m_initModuleIndex >= 10) reminderManager.clearReminders();
    if (m_initModuleIndex >= 9) conversationManager.stopConversation();
    if (m_initModuleIndex >= 8) textToSpeech.stop();
    if (m_initModuleIndex >= 7) geminiClient.cancelRequest();
    if (m_initModuleIndex >= 6) speechToText.cancelRecognition();
    if (m_initModuleIndex >= 5) webPortal.stop();
    if (m_initModuleIndex >= 4) {
        audioManager.stopPlayback();
        audioManager.stopRecording();
    }
    if (m_initModuleIndex >= 3) wifiManager.disconnect();
    if (m_initModuleIndex >= 2) {
        displayManager.clear();
        displayManager.displayOff();
    }
    if (m_initModuleIndex >= 1) storageManager.unmountSPIFFS();
}