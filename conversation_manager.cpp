#include "conversation_manager.h"

ConversationManager conversationManager;

ConversationManager::ConversationManager() noexcept
    : m_initialized(false)
    , m_currentState(ConversationState::IDLE)
    , m_previousState(ConversationState::IDLE)
    , m_lastError(ConversationError::NONE)
    , m_result()
    , m_wakeWordEnabled(true)
    , m_autoSpeak(true)
    , m_autoListen(true)
    , m_continuousConversation(false)
    , m_bargeInEnabled(true)
    , m_conversationTimeoutMs(kDefaultConversationTimeoutMs)
    , m_stateStartTime(0)
    , m_conversationStartTime(0)
    , m_sttTriggered(false)
    , m_geminiTriggered(false)
    , m_ttsTriggered(false) {
    m_history.reserve(kMaxHistoryEntries);
}

ConversationManager::~ConversationManager() noexcept = default;

bool ConversationManager::initialize() noexcept {
    if (m_initialized) return true;

    if (!speechToText.isInitialized()) {
        LOG_ERROR("ConversationManager", "SpeechToText not initialized");
        setError(ConversationError::STT_ERROR);
        return false;
    }

    if (!geminiClient.isInitialized()) {
        LOG_ERROR("ConversationManager", "GeminiClient not initialized");
        setError(ConversationError::GEMINI_ERROR);
        return false;
    }

    if (!textToSpeech.isInitialized()) {
        LOG_ERROR("ConversationManager", "TextToSpeech not initialized");
        setError(ConversationError::TTS_ERROR);
        return false;
    }

    if (!displayManager.isInitialized()) {
        LOG_ERROR("ConversationManager", "DisplayManager not initialized");
        return false;
    }

    m_initialized = true;
    m_lastError = ConversationError::NONE;
    LOG_INFO("ConversationManager", "Initialized");
    return true;
}

void ConversationManager::run() noexcept {
    if (!m_initialized) return;

    handleTimeout();

    switch (m_currentState) {
        case ConversationState::LISTENING:
            handleListening();
            break;
        case ConversationState::TRANSCRIBING:
            handleTranscribing();
            break;
        case ConversationState::THINKING:
            handleThinking();
            break;
        case ConversationState::SPEAKING:
            handleSpeaking();
            break;
        case ConversationState::COMPLETED:
            handleCompletion();
            break;
        case ConversationState::ERROR:
            if (!checkSubmoduleErrors()) {
                changeState(ConversationState::IDLE);
            }
            break;
        case ConversationState::PAUSED:
        case ConversationState::IDLE:
        default:
            break;
    }

    updateDisplay();
}

void ConversationManager::update() noexcept {
    run();
}

bool ConversationManager::startConversation() noexcept {
    if (!m_initialized) {
        setError(ConversationError::UNKNOWN);
        return false;
    }
    if (m_currentState != ConversationState::IDLE && m_currentState != ConversationState::COMPLETED && m_currentState != ConversationState::ERROR) {
        LOG_WARN("ConversationManager", "Cannot start, busy in state %d", static_cast<int>(m_currentState));
        return false;
    }

    resetConversation();
    m_conversationStartTime = millis();
    changeState(ConversationState::LISTENING);
    LOG_INFO("ConversationManager", "Conversation started");
    return true;
}

void ConversationManager::stopConversation() noexcept {
    if (m_currentState == ConversationState::IDLE) return;

    speechToText.cancelRecognition();
    geminiClient.cancelRequest();
    textToSpeech.stop();

    resetConversation();
    changeState(ConversationState::IDLE);
    LOG_INFO("ConversationManager", "Conversation stopped");
}

void ConversationManager::pauseConversation() noexcept {
    if (m_currentState == ConversationState::SPEAKING) {
        textToSpeech.pause();
        m_previousState = ConversationState::SPEAKING;
        changeState(ConversationState::PAUSED);
    } else if (m_currentState == ConversationState::LISTENING || m_currentState == ConversationState::TRANSCRIBING) {
        speechToText.cancelRecognition();
        m_previousState = m_currentState;
        changeState(ConversationState::PAUSED);
    } else if (m_currentState == ConversationState::THINKING) {
        geminiClient.cancelRequest();
        m_previousState = ConversationState::THINKING;
        changeState(ConversationState::PAUSED);
    }
}

void ConversationManager::resumeConversation() noexcept {
    if (m_currentState != ConversationState::PAUSED) return;

    switch (m_previousState) {
        case ConversationState::LISTENING:
        case ConversationState::TRANSCRIBING:
            changeState(ConversationState::LISTENING);
            break;
        case ConversationState::THINKING:
            changeState(ConversationState::THINKING);
            break;
        case ConversationState::SPEAKING:
            textToSpeech.resume();
            changeState(ConversationState::SPEAKING);
            break;
        default:
            changeState(ConversationState::IDLE);
            break;
    }
}

void ConversationManager::cancelConversation() noexcept {
    stopConversation();
}

void ConversationManager::processWakeWord() noexcept {
    if (!m_wakeWordEnabled) return;
    if (m_currentState == ConversationState::IDLE || m_currentState == ConversationState::COMPLETED) {
        startConversation();
    }
}

void ConversationManager::processButtonPress() noexcept {
    if (m_currentState == ConversationState::IDLE || m_currentState == ConversationState::COMPLETED) {
        startConversation();
    } else if (m_currentState == ConversationState::SPEAKING && m_bargeInEnabled) {
        textToSpeech.stop();
        speechToText.cancelRecognition();
        resetConversation();
        changeState(ConversationState::LISTENING);
    } else if (m_currentState == ConversationState::LISTENING || m_currentState == ConversationState::TRANSCRIBING) {
        speechToText.stopRecognition();
    }
}

void ConversationManager::setWakeWordEnabled(bool enabled) noexcept {
    m_wakeWordEnabled = enabled;
}

void ConversationManager::setConversationTimeout(unsigned long timeoutMs) noexcept {
    m_conversationTimeoutMs = timeoutMs;
}

void ConversationManager::setAutoSpeak(bool enabled) noexcept {
    m_autoSpeak = enabled;
}

void ConversationManager::setAutoListen(bool enabled) noexcept {
    m_autoListen = enabled;
}

void ConversationManager::enableContinuousConversation() noexcept {
    m_continuousConversation = true;
}

void ConversationManager::disableContinuousConversation() noexcept {
    m_continuousConversation = false;
}

void ConversationManager::enableBargeIn() noexcept {
    m_bargeInEnabled = true;
}

void ConversationManager::disableBargeIn() noexcept {
    m_bargeInEnabled = false;
}

bool ConversationManager::isBusy() const noexcept {
    return m_currentState != ConversationState::IDLE && m_currentState != ConversationState::COMPLETED && m_currentState != ConversationState::ERROR;
}

bool ConversationManager::isListening() const noexcept {
    return m_currentState == ConversationState::LISTENING || m_currentState == ConversationState::TRANSCRIBING;
}

bool ConversationManager::isSpeaking() const noexcept {
    return m_currentState == ConversationState::SPEAKING;
}

bool ConversationManager::isInitialized() const noexcept {
    return m_initialized;
}

bool ConversationManager::isBargeInEnabled() const noexcept {
    return m_bargeInEnabled;
}

ConversationState ConversationManager::getState() const noexcept {
    return m_currentState;
}

ConversationError ConversationManager::getError() const noexcept {
    return m_lastError;
}

const ConversationResult& ConversationManager::getResult() const noexcept {
    return m_result;
}

const std::vector<HistoryEntry>& ConversationManager::getHistory() const noexcept {
    return m_history;
}

void ConversationManager::clearHistory() noexcept {
    m_history.clear();
}

void ConversationManager::changeState(ConversationState newState) noexcept {
    if (m_currentState == newState) return;

    static constexpr bool validTransition[8][8] = {
        {0,1,0,0,0,0,0,1},  // IDLE -> LISTENING, ERROR
        {0,0,1,0,0,1,0,1},  // LISTENING -> TRANSCRIBING, PAUSED, ERROR
        {0,0,0,1,0,1,0,1},  // TRANSCRIBING -> THINKING, PAUSED, ERROR
        {0,0,0,0,1,1,1,1},  // THINKING -> SPEAKING, PAUSED, COMPLETED, ERROR
        {0,0,0,0,0,1,1,1},  // SPEAKING -> PAUSED, COMPLETED, ERROR
        {1,1,0,1,1,0,0,1},  // PAUSED -> IDLE, LISTENING, THINKING, SPEAKING, ERROR
        {1,1,0,0,0,0,0,1},  // COMPLETED -> IDLE, LISTENING, ERROR
        {1,0,0,0,0,0,0,0}   // ERROR -> IDLE
    };

    if (!validTransition[static_cast<uint8_t>(m_currentState)][static_cast<uint8_t>(newState)]) {
        LOG_WARN("ConversationManager", "Invalid transition %d -> %d", static_cast<int>(m_currentState), static_cast<int>(newState));
        return;
    }

    LOG_DEBUG("ConversationManager", "State %d -> %d", static_cast<int>(m_currentState), static_cast<int>(newState));
    m_currentState = newState;
    m_stateStartTime = millis();
}

void ConversationManager::setError(ConversationError error) noexcept {
    if (m_lastError == error) return;
    m_lastError = error;
    m_result.error = error;
    LOG_ERROR("ConversationManager", "Error %d", static_cast<int>(error));
}

void ConversationManager::resetConversation() noexcept {
    m_result.clear();
    m_lastError = ConversationError::NONE;
    m_sttTriggered = false;
    m_geminiTriggered = false;
    m_ttsTriggered = false;
}

void ConversationManager::storeHistoryEntry() noexcept {
    if (m_result.userText.isEmpty() && m_result.assistantText.isEmpty()) return;

    if (m_history.size() >= kMaxHistoryEntries) {
        m_history.erase(m_history.begin());
    }
    m_history.emplace_back(m_result.userText, m_result.assistantText, m_result.timestamp, m_result.latencyMs);
}

void ConversationManager::handleListening() noexcept {
    if (!m_sttTriggered) {
        if (speechToText.startRecognition(RecognitionMode::ONESHOT)) {
            m_sttTriggered = true;
            LOG_INFO("ConversationManager", "STT started");
        } else {
            setError(ConversationError::STT_ERROR);
            changeState(ConversationState::ERROR);
        }
        return;
    }

    if (!speechToText.isBusy()) {
        const SpeechResult& sttResult = speechToText.getResult();
        if (sttResult.error != SpeechError::NONE) {
            setError(ConversationError::STT_ERROR);
            changeState(ConversationState::ERROR);
            return;
        }

        m_result.userText = sttResult.transcript;
        m_result.timestamp = millis();
        speechToText.clearResult();
        m_sttTriggered = false;

        if (m_result.userText.isEmpty()) {
            LOG_WARN("ConversationManager", "Empty transcript");
            if (m_continuousConversation) {
                changeState(ConversationState::LISTENING);
            } else {
                changeState(ConversationState::COMPLETED);
            }
            return;
        }

        LOG_INFO("ConversationManager", "User said: %s", m_result.userText.c_str());
        changeState(ConversationState::TRANSCRIBING);
    }
}

void ConversationManager::handleTranscribing() noexcept {
    if (!m_geminiTriggered) {
        if (geminiClient.sendPrompt(m_result.userText)) {
            m_geminiTriggered = true;
            LOG_INFO("ConversationManager", "Gemini request sent");
        } else {
            setError(ConversationError::GEMINI_ERROR);
            changeState(ConversationState::ERROR);
        }
        return;
    }

    if (!geminiClient.isBusy()) {
        const GeminiResponse& geminiResult = geminiClient.getResponse();
        if (geminiResult.error != GeminiError::NONE) {
            setError(ConversationError::GEMINI_ERROR);
            changeState(ConversationState::ERROR);
            return;
        }

        m_result.assistantText = geminiResult.responseText;
        geminiClient.clearResponse();
        m_geminiTriggered = false;

        LOG_INFO("ConversationManager", "Gemini responded: %s", m_result.assistantText.c_str());

        if (m_autoSpeak && !m_result.assistantText.isEmpty()) {
            changeState(ConversationState::SPEAKING);
        } else {
            changeState(ConversationState::COMPLETED);
        }
    }
}

void ConversationManager::handleThinking() noexcept {
    handleTranscribing();
}

void ConversationManager::handleSpeaking() noexcept {
    if (!m_ttsTriggered) {
        if (textToSpeech.speak(m_result.assistantText)) {
            m_ttsTriggered = true;
            LOG_INFO("ConversationManager", "TTS started");
        } else {
            setError(ConversationError::TTS_ERROR);
            changeState(ConversationState::ERROR);
        }
        return;
    }

    if (!textToSpeech.isBusy()) {
        m_ttsTriggered = false;
        LOG_INFO("ConversationManager", "TTS completed");
        changeState(ConversationState::COMPLETED);
    }
}

void ConversationManager::handleCompletion() noexcept {
    m_result.latencyMs = millis() - m_conversationStartTime;
    m_result.timestamp = millis();
    m_result.error = ConversationError::NONE;

    storeHistoryEntry();

    LOG_INFO("ConversationManager", "Turn completed (%d ms)", m_result.latencyMs);

    if (m_continuousConversation && m_autoListen) {
        resetConversation();
        changeState(ConversationState::LISTENING);
    } else {
        changeState(ConversationState::IDLE);
    }
}

void ConversationManager::handleTimeout() noexcept {
    if (m_conversationTimeoutMs == 0) return;

    unsigned long now = millis();

    if (m_currentState != ConversationState::IDLE && m_currentState != ConversationState::COMPLETED && m_currentState != ConversationState::ERROR && m_currentState != ConversationState::PAUSED) {
        if (now - m_conversationStartTime >= m_conversationTimeoutMs) {
            LOG_WARN("ConversationManager", "Conversation timeout");
            setError(ConversationError::TIMEOUT);
            stopConversation();
            return;
        }
    }

    if (m_currentState != ConversationState::IDLE && m_currentState != ConversationState::COMPLETED && m_currentState != ConversationState::ERROR && m_currentState != ConversationState::PAUSED) {
        if (now - m_stateStartTime >= kStateTimeoutMs) {
            LOG_WARN("ConversationManager", "State %d timeout", static_cast<int>(m_currentState));
            setError(ConversationError::TIMEOUT);
            changeState(ConversationState::ERROR);
        }
    }
}

void ConversationManager::updateDisplay() noexcept {
    static DisplayState lastDisplayState = DisplayState::HOME;

    DisplayState targetState = DisplayState::HOME;
    bool forceUpdate = false;

    switch (m_currentState) {
        case ConversationState::IDLE:
            targetState = DisplayState::HOME;
            break;
        case ConversationState::LISTENING:
            targetState = DisplayState::LISTENING;
            break;
        case ConversationState::TRANSCRIBING:
        case ConversationState::THINKING:
            targetState = DisplayState::THINKING;
            break;
        case ConversationState::SPEAKING:
            targetState = DisplayState::SPEAKING;
            break;
        case ConversationState::ERROR:
            targetState = DisplayState::ERROR;
            forceUpdate = true;
            break;
        default:
            break;
    }

    if (targetState != lastDisplayState || forceUpdate) {
        switch (targetState) {
            case DisplayState::HOME:
                displayManager.showHome();
                break;
            case DisplayState::LISTENING:
                displayManager.showListening();
                break;
            case DisplayState::THINKING:
                displayManager.showThinking();
                break;
            case DisplayState::SPEAKING:
                displayManager.showSpeaking();
                break;
            case DisplayState::ERROR:
                displayManager.showError("Error", "Conversation failed");
                break;
            default:
                break;
        }
        lastDisplayState = targetState;
    }
}

bool ConversationManager::checkSubmoduleErrors() noexcept {
    if (speechToText.getError() != SpeechError::NONE) return true;
    if (geminiClient.getError() != GeminiError::NONE) return true;
    if (textToSpeech.getError() != TTSError::NONE) return true;
    return false;
}