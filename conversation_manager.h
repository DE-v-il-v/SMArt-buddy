#ifndef AURA_CONVERSATION_MANAGER_H
#define AURA_CONVERSATION_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "speech_to_text.h"
#include "gemini_client.h"
#include "text_to_speech.h"
#include "display_manager.h"
#include "logger.h"
#include "config.h"

/**
 * @enum ConversationState
 * @brief Conversation workflow states
 */
enum class ConversationState : uint8_t {
    IDLE,           ///< Waiting for wake word or button
    LISTENING,      ///< SpeechToText recording
    TRANSCRIBING,   ///< SpeechToText processing
    THINKING,       ///< GeminiClient processing
    SPEAKING,       ///< TextToSpeech playing
    PAUSED,         ///< Conversation paused
    COMPLETED,      ///< Turn completed
    ERROR           ///< Error occurred
};

/**
 * @enum ConversationError
 * @brief Conversation error codes
 */
enum class ConversationError : uint8_t {
    NONE,           ///< No error
    STT_ERROR,      ///< Speech-to-text failure
    GEMINI_ERROR,   ///< Gemini API failure
    TTS_ERROR,      ///< Text-to-speech failure
    TIMEOUT,        ///< Conversation timeout
    NETWORK,        ///< Network failure
    UNKNOWN         ///< Unspecified error
};

/**
 * @struct ConversationResult
 * @brief Conversation turn result
 */
struct ConversationResult {
    String userText;           ///< Recognized user speech
    String assistantText;      ///< Generated assistant response
    unsigned long latencyMs;   ///< End-to-end latency
    unsigned long timestamp;   ///< Completion timestamp
    ConversationError error;   ///< Error code if failed

    ConversationResult() noexcept
        : userText(""), assistantText(""), latencyMs(0), timestamp(0), error(ConversationError::NONE) {}

    void clear() noexcept {
        userText.clear();
        assistantText.clear();
        latencyMs = 0;
        timestamp = 0;
        error = ConversationError::NONE;
    }
};

/**
 * @struct HistoryEntry
 * @brief Single conversation history entry
 */
struct HistoryEntry {
    String userText;
    String assistantText;
    unsigned long timestamp;
    unsigned long latencyMs;

    HistoryEntry() noexcept : userText(""), assistantText(""), timestamp(0), latencyMs(0) {}
    HistoryEntry(const String& u, const String& a, unsigned long t, unsigned long l) noexcept
        : userText(u), assistantText(a), timestamp(t), latencyMs(l) {}
};

/**
 * @class ConversationManager
 * @brief Single authority for conversation workflow orchestration
 *
 * Coordinates:
 * - SpeechToText for recognition
 * - GeminiClient for AI responses
 * - TextToSpeech for synthesis
 * - DisplayManager for visual feedback
 *
 * Non-blocking, production-quality conversation manager for ESP32.
 */
class ConversationManager {
public:
    /**
     * @brief Constructor
     */
    ConversationManager() noexcept;

    /**
     * @brief Destructor
     */
    ~ConversationManager() noexcept;

    // Delete copy semantics
    ConversationManager(const ConversationManager&) = delete;
    ConversationManager& operator=(const ConversationManager&) = delete;

    // Delete move semantics
    ConversationManager(ConversationManager&&) = delete;
    ConversationManager& operator=(ConversationManager&&) = delete;

    /**
     * @brief Initialize conversation manager
     * @return true if initialization successful
     * @note Must be called after all submodules are initialized
     */
    [[nodiscard]] bool initialize() noexcept;

    /**
     * @brief Main update loop - process state machine
     * @note Call regularly from loop(), non-blocking
     */
    void run() noexcept;

    /**
     * @brief Alias for run() for scheduler compatibility
     */
    void update() noexcept;

    /**
     * @brief Start new conversation turn
     * @return true if started successfully
     */
    [[nodiscard]] bool startConversation() noexcept;

    /**
     * @brief Stop current conversation
     */
    void stopConversation() noexcept;

    /**
     * @brief Pause conversation
     */
    void pauseConversation() noexcept;

    /**
     * @brief Resume paused conversation
     */
    void resumeConversation() noexcept;

    /**
     * @brief Cancel conversation and return to IDLE
     */
    void cancelConversation() noexcept;

    /**
     * @brief Process wake word detection
     * @note Called by wake word detector
     */
    void processWakeWord() noexcept;

    /**
     * @brief Process button press
     * @note Called by button handler
     */
    void processButtonPress() noexcept;

    /**
     * @brief Enable/disable wake word activation
     * @param enabled True to enable wake word
     */
    void setWakeWordEnabled(bool enabled) noexcept;

    /**
     * @brief Set conversation timeout
     * @param timeoutMs Timeout in milliseconds (0 = no timeout)
     */
    void setConversationTimeout(unsigned long timeoutMs) noexcept;

    /**
     * @brief Enable/disable auto-speak after Gemini response
     * @param enabled True to auto-speak
     */
    void setAutoSpeak(bool enabled) noexcept;

    /**
     * @brief Enable/disable auto-listen after TTS completes
     * @param enabled True to auto-listen
     */
    void setAutoListen(bool enabled) noexcept;

    /**
     * @brief Enable continuous conversation mode
     */
    void enableContinuousConversation() noexcept;

    /**
     * @brief Disable continuous conversation mode
     */
    void disableContinuousConversation() noexcept;

    /**
     * @brief Enable barge-in (interrupt TTS with new speech)
     */
    void enableBargeIn() noexcept;

    /**
     * @brief Disable barge-in
     */
    void disableBargeIn() noexcept;

    /**
     * @brief Check if conversation is active
     * @return true if not IDLE
     */
    [[nodiscard]] bool isBusy() const noexcept;

    /**
     * @brief Check if currently listening
     * @return true if in LISTENING or TRANSCRIBING state
     */
    [[nodiscard]] bool isListening() const noexcept;

    /**
     * @brief Check if currently speaking
     * @return true if in SPEAKING state
     */
    [[nodiscard]] bool isSpeaking() const noexcept;

    /**
     * @brief Check if module is initialized
     * @return true if initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept;

    /**
     * @brief Check if barge-in is enabled
     * @return true if enabled
     */
    [[nodiscard]] bool isBargeInEnabled() const noexcept;

    /**
     * @brief Get current state
     * @return Current ConversationState
     */
    [[nodiscard]] ConversationState getState() const noexcept;

    /**
     * @brief Get last error code
     * @return Current ConversationError
     */
    [[nodiscard]] ConversationError getError() const noexcept;

    /**
     * @brief Get last conversation result
     * @return Const reference to ConversationResult
     */
    [[nodiscard]] const ConversationResult& getResult() const noexcept;

    /**
     * @brief Get conversation history
     * @return Const reference to history vector
     */
    [[nodiscard]] const std::vector<HistoryEntry>& getHistory() const noexcept;

    /**
     * @brief Clear conversation history
     */
    void clearHistory() noexcept;

private:
    // State management
    void changeState(ConversationState newState) noexcept;
    void setError(ConversationError error) noexcept;
    void resetConversation() noexcept;
    void storeHistoryEntry() noexcept;

    // Workflow handlers
    void handleListening() noexcept;
    void handleTranscribing() noexcept;
    void handleThinking() noexcept;
    void handleSpeaking() noexcept;
    void handleCompletion() noexcept;
    void handleTimeout() noexcept;

    // Helpers
    void updateDisplay() noexcept;
    bool checkSubmoduleErrors() noexcept;

    // Member variables
    bool m_initialized;
    ConversationState m_currentState;
    ConversationState m_previousState;
    ConversationError m_lastError;
    ConversationResult m_result;

    // Configuration
    bool m_wakeWordEnabled;
    bool m_autoSpeak;
    bool m_autoListen;
    bool m_continuousConversation;
    bool m_bargeInEnabled;
    unsigned long m_conversationTimeoutMs;

    // Timing
    unsigned long m_stateStartTime;
    unsigned long m_conversationStartTime;

    // Submodule interaction flags
    bool m_sttTriggered;
    bool m_geminiTriggered;
    bool m_ttsTriggered;

    // Conversation history
    std::vector<HistoryEntry> m_history;
    static constexpr size_t kMaxHistoryEntries = 10;

    // Constants
    static constexpr unsigned long kDefaultConversationTimeoutMs = 60000UL;
    static constexpr unsigned long kStateTimeoutMs = 30000UL;
};

/**
 * @brief Global conversation manager instance
 */
extern ConversationManager conversationManager;

#endif // AURA_CONVERSATION_MANAGER_H