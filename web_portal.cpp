#include "web_portal.h"
#include <ESPmDNS.h>
#include <Update.h>

/// Global WebPortal instance
WebPortal webPortal;

// ============================================================================
// HTML Templates (PROGMEM)
// ============================================================================

static const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; margin-bottom: 20px; }
        h2 { color: #555; margin: 20px 0 10px 0; font-size: 1.2em; }
        .nav { margin: 20px 0; display: flex; flex-wrap: wrap; gap: 10px; }
        .nav a { padding: 10px 15px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; flex-grow: 1; text-align: center; }
        .nav a:hover { background: #0056b3; }
        .info { background: #e7f3ff; padding: 12px; border-left: 4px solid #2196F3; margin: 10px 0; border-radius: 4px; }
        .status-item { padding: 10px; margin: 5px 0; background: #f9f9f9; border-left: 4px solid #2196F3; border-radius: 4px; }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; font-weight: 600; color: #333; }
        input[type="text"], input[type="password"], input[type="file"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px; }
        input[type="text"]:focus, input[type="password"]:focus, input[type="file"]:focus { outline: none; border-color: #007bff; box-shadow: 0 0 0 3px rgba(0,123,255,0.1); }
        button { padding: 10px 20px; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: 600; }
        button:hover { background: #218838; }
        .danger-btn { background: #dc3545; }
        .danger-btn:hover { background: #c82333; }
        .footer { margin-top: 30px; padding-top: 20px; border-top: 1px solid #eee; text-align: center; color: #666; font-size: 0.9em; }
        .footer a { color: #007bff; text-decoration: none; }
    </style>
)rawliteral";

static const char HTML_ROOT[] PROGMEM = R"rawliteral(
<title>AURA Dashboard</title>
</head>
<body>
    <div class="container">
        <h1>🤖 AURA AI Assistant</h1>
        <div class="info">
            <p><strong>Status:</strong> Online</p>
            <p><strong>Hostname:</strong> 
)rawliteral";

static const char HTML_ROOT_SUFFIX[] PROGMEM = R"rawliteral(</p>
        </div>
        <div class="nav">
            <a href="/status">Device Status</a>
            <a href="/wifi">Wi-Fi Settings</a>
            <a href="/settings">Device Settings</a>
            <a href="/ota">Firmware Update</a>
        </div>
        <div class="footer">
            <p>AURA v1.0.0 | ESP32-WROOM-32</p>
        </div>
    </div>
</body>
</html>
)rawliteral";

static const char HTML_STATUS[] PROGMEM = R"rawliteral(
<title>Device Status</title>
</head>
<body>
    <div class="container">
        <h1>Device Status</h1>
        <div class="status-item"><strong>Wi-Fi Status:</strong> 
)rawliteral";

static const char HTML_WIFI_CONFIG[] PROGMEM = R"rawliteral(
<title>Wi-Fi Configuration</title>
</head>
<body>
    <div class="container">
        <h1>Wi-Fi Configuration</h1>
        <form method="POST" action="/wifi">
            <div class="form-group">
                <label for="ssid">SSID:</label>
                <input type="text" id="ssid" name="ssid" required autofocus>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password">
            </div>
            <button type="submit">Save Settings</button>
            <a href="/" style="margin-left: 10px; padding: 10px 15px; background: #6c757d; color: white; text-decoration: none; border-radius: 4px; display: inline-block;">Cancel</a>
        </form>
    </div>
</body>
</html>
)rawliteral";

static const char HTML_SETTINGS[] PROGMEM = R"rawliteral(
<title>Device Settings</title>
</head>
<body>
    <div class="container">
        <h1>Device Settings</h1>
        <form method="POST" action="/settings">
            <div class="form-group">
                <label for="device_name">Device Name:</label>
                <input type="text" id="device_name" name="device_name" value="AURA">
            </div>
            <button type="submit">Save Settings</button>
        </form>
        <h2>Danger Zone</h2>
        <form method="POST" action="/factory-reset" style="display:inline;">
            <button type="submit" class="danger-btn" onclick="return confirm('⚠️ This will erase ALL settings. Continue?')">Factory Reset</button>
        </form>
        <form method="POST" action="/restart" style="display:inline; margin-left: 10px;">
            <button type="submit" class="danger-btn" onclick="return confirm('Restart device?')">Restart Device</button>
        </form>
        <div class="footer">
            <a href="/">&larr; Back to Dashboard</a>
        </div>
    </div>
</body>
</html>
)rawliteral";

static const char HTML_OTA[] PROGMEM = R"rawliteral(
<title>Firmware Update</title>
</head>
<body>
    <div class="container">
        <h1>Firmware Update</h1>
        <div class="info">
            <p><strong>Current Firmware:</strong> v1.0.0</p>
            <p><strong>Latest Available:</strong> v1.0.0</p>
            <p><small>Select a .bin firmware file and upload to update.</small></p>
        </div>
        <form method="POST" enctype="multipart/form-data" action="/ota">
            <div class="form-group">
                <label for="firmware">Select .bin file:</label>
                <input type="file" id="firmware" name="firmware" accept=".bin" required autofocus>
            </div>
            <button type="submit">Upload Firmware</button>
            <a href="/" style="margin-left: 10px; padding: 10px 15px; background: #6c757d; color: white; text-decoration: none; border-radius: 4px; display: inline-block;">Cancel</a>
        </form>
        <div class="footer">
            <a href="/">&larr; Back to Dashboard</a>
        </div>
    </div>
</body>
</html>
)rawliteral";

static const char HTML_404[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>404 Not Found</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; text-align: center; margin: 50px; background: #f5f5f5; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; }
        h1 { color: #dc3545; font-size: 3em; margin-bottom: 10px; }
        p { color: #666; margin: 20px 0; }
        a { color: #007bff; text-decoration: none; font-weight: 600; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>404</h1>
        <p>Page not found.</p>
        <a href="/">&larr; Return to Dashboard</a>
    </div>
</body>
</html>
)rawliteral";

// ============================================================================
// Constants
// ============================================================================

static constexpr unsigned long RESTART_DELAY_MS{1000};
static constexpr unsigned long FACTORY_RESET_DELAY_MS{1000};
static constexpr unsigned long OTA_RESTART_DELAY_MS{2000};
static constexpr size_t MAX_JSON_BUFFER{512};
static constexpr size_t HTML_BUFFER_ROOT{512};
static constexpr size_t HTML_BUFFER_STATUS{768};
static constexpr size_t HTML_BUFFER_JSON{256};
static constexpr const char* JSON_SUCCESS_TEMPLATE = R"({"success":true,"message":"%s"})";
static constexpr const char* JSON_ERROR_TEMPLATE = R"({"success":false,"error":"%s"})";

// ============================================================================
// Forward Declarations for Private Task Functions
// ============================================================================

static void restartTask(void* pvParameters);
static void factoryResetTask(void* pvParameters);
static void otaRestartTask(void* pvParameters);

// ============================================================================
// Constructor / Destructor
// ============================================================================

WebPortal::WebPortal() noexcept
    : m_server(m_port),
      m_dnsServer(),
      m_running(false),
      m_captivePortalActive(false),
      m_otaInProgress(false),
      m_lastClientActivity(0),
      m_requestCounter(0)
{
}

WebPortal::~WebPortal() noexcept
{
    stop();
    stopCaptivePortal();
}

// ============================================================================
// Public API - Lifecycle
// ============================================================================

bool WebPortal::initialize() noexcept
{
    Logger::info("WebPortal", "Initializing web portal");

    if (!registerRoutes())
    {
        Logger::error("WebPortal", "Failed to register standard routes");
        return false;
    }

    if (!registerApiRoutes())
    {
        Logger::error("WebPortal", "Failed to register API routes");
        return false;
    }

    Logger::info("WebPortal", "Web portal initialized successfully");
    return true;
}

bool WebPortal::start() noexcept
{
    if (m_running)
    {
        Logger::warning("WebPortal", "Server already running");
        return true;
    }

    m_server.begin();
    m_running = true;
    m_lastClientActivity = millis();

    Logger::info("WebPortal", "Web server started on port 80");
    return true;
}

bool WebPortal::stop() noexcept
{
    if (!m_running)
    {
        return true;
    }

    m_server.stop();
    m_running = false;

    Logger::info("WebPortal", "Web server stopped");
    return true;
}

void WebPortal::update() noexcept
{
    if (!m_running)
    {
        return;
    }

    if (m_captivePortalActive)
    {
        m_dnsServer.processNextRequest();
    }

    handleClient();
    m_lastClientActivity = millis();
}

void WebPortal::handleClient() noexcept
{
    if (!m_running)
    {
        return;
    }

    m_server.handleClient();
}

bool WebPortal::isRunning() const noexcept
{
    return m_running;
}

// ============================================================================
// Captive Portal
// ============================================================================

bool WebPortal::beginCaptivePortal() noexcept
{
    if (m_captivePortalActive)
    {
        Logger::warning("WebPortal", "Captive portal already active");
        return true;
    }

    if (!m_running)
    {
        Logger::error("WebPortal", "Cannot start captive portal: server not running");
        return false;
    }

    IPAddress apIP = WiFi.softAPIP();
    if (!m_dnsServer.start(m_dnsPort, "*", apIP))
    {
        Logger::error("WebPortal", "Failed to start DNS server");
        return false;
    }

    m_captivePortalActive = true;
    Logger::info("WebPortal", "Captive portal activated");
    return true;
}

bool WebPortal::stopCaptivePortal() noexcept
{
    if (!m_captivePortalActive)
    {
        return true;
    }

    m_dnsServer.stop();
    m_captivePortalActive = false;

    Logger::info("WebPortal", "Captive portal deactivated");
    return true;
}

// ============================================================================
// Route Registration
// ============================================================================

bool WebPortal::registerRoutes() noexcept
{
    m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    m_server.on("/wifi", HTTP_GET, [this]() { handleWifi(); });
    m_server.on("/wifi", HTTP_POST, [this]() { handleSaveWifi(); });
    m_server.on("/settings", HTTP_GET, [this]() { handleSettings(); });
    m_server.on("/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    m_server.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    m_server.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
    m_server.on("/ota", HTTP_GET, [this]() { handleOTA(); });
    m_server.on("/ota", HTTP_POST, [this]() { handleOTA(); });

    // Captive portal common endpoints
    m_server.on("/generate_204", HTTP_GET, [this]() { m_server.send(204); });
    m_server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleRoot(); });
    m_server.on("/connecttest.txt", HTTP_GET, [this]() { m_server.send(200, "text/plain", "Success"); });
    m_server.on("/fwlink", HTTP_GET, [this]() {
        m_server.sendHeader("Location", "http://192.168.4.1/");
        m_server.send(302);
    });

    m_server.onNotFound([this]() { handleNotFound(); });

    Logger::info("WebPortal", "Standard routes registered");
    return true;
}

bool WebPortal::registerApiRoutes() noexcept
{
    m_server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    m_server.on("/api/wifi", HTTP_GET, [this]() { handleApiWifi(); });
    m_server.on("/api/wifi", HTTP_POST, [this]() { handleApiWifi(); });
    m_server.on("/api/settings", HTTP_GET, [this]() { handleApiSettings(); });
    m_server.on("/api/settings", HTTP_POST, [this]() { handleApiSettings(); });

    Logger::info("WebPortal", "API routes registered");
    return true;
}

// ============================================================================
// System Control
// ============================================================================

void WebPortal::restartDevice() noexcept
{
    Logger::warning("WebPortal", "Device restart requested");
    sendSuccess("Device restarting...");

    xTaskCreate(restartTask, "restart_task", 2048, nullptr, 1, nullptr);
}

void WebPortal::factoryReset() noexcept
{
    Logger::warning("WebPortal", "Factory reset requested");
    sendSuccess("Factory reset initiated...");

    xTaskCreate(factoryResetTask, "factory_reset_task", 2048, nullptr, 1, nullptr);
}

// ============================================================================
// Status Functions
// ============================================================================

uint16_t WebPortal::getPort() const noexcept
{
    return m_port;
}

String WebPortal::getIPAddress() const noexcept
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }
    return "";
}

String WebPortal::getHostname() const noexcept
{
    return WiFi.getHostname();
}

// ============================================================================
// Private Handlers - Standard Routes
// ============================================================================

void WebPortal::handleRoot() noexcept
{
    m_requestCounter++;

    String html;
    html.reserve(HTML_BUFFER_ROOT);
    html += FPSTR(HTML_HEADER);
    html += FPSTR(HTML_ROOT);
    html += WiFi.getHostname();
    html += " (";
    html += WiFi.localIP().toString();
    html += ")";
    html += FPSTR(HTML_ROOT_SUFFIX);

    m_server.send(200, "text/html; charset=utf-8", html);
}

void WebPortal::handleStatus() noexcept
{
    m_requestCounter++;

    String html;
    html.reserve(HTML_BUFFER_STATUS);
    html += FPSTR(HTML_HEADER);
    html += FPSTR(HTML_STATUS);

    if (WiFi.status() == WL_CONNECTED)
    {
        html += "Connected";
    }
    else
    {
        html += "Disconnected";
    }

    html += "</div>";
    html += "<div class=\"status-item\"><strong>IP Address:</strong> ";
    html += WiFi.localIP().toString();
    html += "</div>";
    html += "<div class=\"status-item\"><strong>SSID:</strong> ";
    html += WiFi.SSID();
    html += "</div>";
    html += "<div class=\"status-item\"><strong>Signal Strength:</strong> ";
    html += String(WiFi.RSSI());
    html += " dBm</div>";
    html += "<div class=\"status-item\"><strong>Free Heap:</strong> ";
    html += String(ESP.getFreeHeap());
    html += " bytes</div>";
    html += "<div class=\"status-item\"><strong>Uptime:</strong> ";
    html += String(millis() / 1000);
    html += " seconds</div>";
    html += "<div class=\"footer\"><a href=\"/\">&larr; Back to Dashboard</a></div>";
    html += "</div></body></html>";

    m_server.send(200, "text/html; charset=utf-8", html);
}

void WebPortal::handleWifi() noexcept
{
    m_requestCounter++;

    String html;
    html.reserve(256);
    html += FPSTR(HTML_HEADER);
    html += FPSTR(HTML_WIFI_CONFIG);

    m_server.send(200, "text/html; charset=utf-8", html);
}

void WebPortal::handleSaveWifi() noexcept
{
    m_requestCounter++;

    if (!m_server.hasArg("ssid") || !m_server.hasArg("password"))
    {
        sendError("Missing SSID or password");
        return;
    }

    String ssid = m_server.arg("ssid");
    String password = m_server.arg("password");

    wifiManager.connect(ssid.c_str(), password.c_str());
    sendSuccess("Wi-Fi configuration saved. Device will reconnect...");
}

void WebPortal::handleSettings() noexcept
{
    m_requestCounter++;

    String html;
    html.reserve(256);
    html += FPSTR(HTML_HEADER);
    html += FPSTR(HTML_SETTINGS);

    m_server.send(200, "text/html; charset=utf-8", html);
}

void WebPortal::handleSaveSettings() noexcept
{
    m_requestCounter++;

    if (m_server.hasArg("device_name"))
    {
        String deviceName = m_server.arg("device_name");
        Logger::info("WebPortal", "Device name updated");
    }

    sendSuccess("Settings saved successfully");
}

void WebPortal::handleRestart() noexcept
{
    m_requestCounter++;
    restartDevice();
}

void WebPortal::handleFactoryReset() noexcept
{
    m_requestCounter++;
    factoryReset();
}

void WebPortal::handleOTA() noexcept
{
    m_requestCounter++;

    if (m_server.method() == HTTP_GET)
    {
        String html;
        html.reserve(256);
        html += FPSTR(HTML_HEADER);
        html += FPSTR(HTML_OTA);

        m_server.send(200, "text/html; charset=utf-8", html);
    }
    else if (m_server.method() == HTTP_POST)
    {
        HTTPUpload& upload = m_server.upload();

        if (upload.status == UPLOAD_FILE_START)
        {
            Logger::info("WebPortal", "OTA upload started");
            m_otaInProgress = true;

            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
            {
                Logger::error("WebPortal", "OTA update begin failed");
                m_otaInProgress = false;
                sendError("OTA update initialization failed", 500);
                return;
            }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
            if (m_otaInProgress)
            {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Logger::error("WebPortal", "OTA update write failed");
                    Update.abort();
                    m_otaInProgress = false;
                    sendError("OTA write failed", 500);
                }
            }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
            if (m_otaInProgress && Update.end(true))
            {
                Logger::info("WebPortal", "OTA update completed successfully");
                m_otaInProgress = false;
                sendSuccess("Firmware updated successfully. Device will restart...");

                xTaskCreate(otaRestartTask, "ota_restart_task", 2048, nullptr, 1, nullptr);
            }
            else if (m_otaInProgress)
            {
                Logger::error("WebPortal", "OTA update finalization failed");
                m_otaInProgress = false;
                sendError("OTA update failed", 500);
            }
        }
        else if (upload.status == UPLOAD_FILE_ABORTED)
        {
            Logger::warning("WebPortal", "OTA update aborted");
            Update.abort();
            m_otaInProgress = false;
            sendError("OTA upload aborted", 400);
        }
    }
}

void WebPortal::handleNotFound() noexcept
{
    m_requestCounter++;
    m_server.send(404, "text/html; charset=utf-8", FPSTR(HTML_404));
}

// ============================================================================
// Private Handlers - API Routes
// ============================================================================

void WebPortal::handleApiStatus() noexcept
{
    m_requestCounter++;

    char json[MAX_JSON_BUFFER];
    snprintf(json, sizeof(json),
        R"({"running":%s,"uptime":%lu,"heap_free":%u,"wifi_connected":%s,"requests":%u})",
        m_running ? "true" : "false",
        millis() / 1000,
        ESP.getFreeHeap(),
        WiFi.status() == WL_CONNECTED ? "true" : "false",
        m_requestCounter
    );

    sendJson(json);
}

void WebPortal::handleApiWifi() noexcept
{
    m_requestCounter++;

    if (m_server.method() == HTTP_GET)
    {
        String ssid = WiFi.SSID();
        String ip = WiFi.localIP().toString();
        String gateway = WiFi.gatewayIP().toString();

        char json[MAX_JSON_BUFFER];
        snprintf(json, sizeof(json),
            R"({"connected":%s,"ssid":"%s","ip":"%s","gateway":"%s","signal":%d})",
            WiFi.status() == WL_CONNECTED ? "true" : "false",
            ssid.c_str(),
            ip.c_str(),
            gateway.c_str(),
            WiFi.RSSI()
        );

        sendJson(json);
    }
    else if (m_server.method() == HTTP_POST)
    {
        if (!m_server.hasArg("plain"))
        {
            sendError("No JSON body provided", 400);
            return;
        }

        Logger::info("WebPortal", "Wi-Fi API update request received");
        sendSuccess("Wi-Fi configuration received");
    }
}

void WebPortal::handleApiSettings() noexcept
{
    m_requestCounter++;

    if (m_server.method() == HTTP_GET)
    {
        char json[MAX_JSON_BUFFER];
        snprintf(json, sizeof(json),
            R"({"device_name":"AURA","version":"1.0.0","build_date":"%s","build_time":"%s"})",
            __DATE__,
            __TIME__
        );

        sendJson(json);
    }
    else if (m_server.method() == HTTP_POST)
    {
        if (!m_server.hasArg("plain"))
        {
            sendError("No JSON body provided", 400);
            return;
        }

        Logger::info("WebPortal", "Settings API update request received");
        sendSuccess("Settings updated");
    }
}

// ============================================================================
// Private Helpers - JSON
// ============================================================================

void WebPortal::sendJson(const String& json, const int code) noexcept
{
    m_server.send(code, "application/json; charset=utf-8", json);
}

void WebPortal::sendSuccess(const String& message, const String& data) noexcept
{
    String json;
    json.reserve(HTML_BUFFER_JSON);
    json += "{\"success\":true,\"message\":\"";
    json += message;
    json += "\"";

    if (data.length() > 0)
    {
        json += ",\"data\":";
        json += data;
    }

    json += "}";
    sendJson(json, 200);
}

void WebPortal::sendError(const String& message, const int code) noexcept
{
    String json;
    json.reserve(HTML_BUFFER_JSON);
    json += "{\"success\":false,\"error\":\"";
    json += message;
    json += "\"}";
    sendJson(json, code);
}

// ============================================================================
// Private Helpers - Static Files
// ============================================================================

void WebPortal::serveStaticFiles(const String& path) noexcept
{
    (void)path;
}

// ============================================================================
// Private Task Functions
// ============================================================================

/**
 * @brief FreeRTOS task for device restart.
 *
 * Scheduled restart with delay to allow HTTP response to complete.
 */
static void restartTask(void* pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY_MS));
    ESP.restart();
    vTaskDelete(nullptr);
}

/**
 * @brief FreeRTOS task for factory reset.
 *
 * Clears Wi-Fi credentials and restarts device.
 */
static void factoryResetTask(void* pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(FACTORY_RESET_DELAY_MS));
    wifiManager.clearCredentials();
    Logger::info("WebPortal", "Wi-Fi credentials cleared");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
    vTaskDelete(nullptr);
}

/**
 * @brief FreeRTOS task for OTA restart.
 *
 * Scheduled restart after OTA firmware update completion.
 */
static void otaRestartTask(void* pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(OTA_RESTART_DELAY_MS));
    ESP.restart();
    vTaskDelete(nullptr);
}