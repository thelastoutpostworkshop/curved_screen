// Web server functions

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "configure.h"
#include "webserver.h"
#include "secrets.h"
#include "slaves.h"

const char *homePageTemplate = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Home Page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; padding: 0; background-color: #f0f0f0; color: #333; }
    h1 { color: #007BFF; }
    #slaveCount { background-color: #007BFF; color: #ffffff; padding: 10px; border-radius: 5px; display: inline-block; }
  </style>
</head>
<body>
  <h1>Sphere Master Control</h1>
  <p>Number of ESP32 Slaves: <span id="slaveCount">%SLAVE_COUNT%</span></p>
</body>
</html>
)rawliteral";

ErrorCode lastError; // Contain the last error code

#ifdef MASTER
extern SLAVES slaves; // Slaves

AsyncWebServer masterServer(masterPort);
bool masterReady = false; // Master is ready to receive calibration data from slaves

// Function to process the calibration data
void handleCalibrationData(uint8_t *data, size_t len, AsyncWebServerRequest *request)
{
    String calibrationData;
    for (size_t i = 0; i < len; i++)
    {
        calibrationData += (char)data[i];
    }

    Serial.println("Received calibration data:");
    Serial.println(calibrationData);
    slaves.addCalibrationData(calibrationData);
    request->send(200, "text/plain", "Calibration data received");
}
void handleReady(AsyncWebServerRequest *request)
{
    // Convert the boolean to a string representation
    String response = masterReady ? "true" : "false";

    // Send the response as plain text
    request->send(200, "text/plain", response);
}
#endif

ErrorCode initWebServer()
{
    Serial.println("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    // WiFi.setAutoReconnect(true);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

#ifdef MASTER
    // The master web server is started with its own name
    if (!MDNS.begin(MASTER_SERVERNAME))
    {
        Serial.println("Error starting mDNS for the Master");
        return noMDNS;
    }
    masterServer.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
                    { handleReady(request); });
    masterServer.on(
        "/calibration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            handleCalibrationData(data, len, request);
        });
    masterServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                    {
  String page = String(homePageTemplate);
  page.replace("%SLAVE_COUNT%", String(SLAVECOUNT));
  request->send(200, "text/html", page); });
    masterServer.begin();
    Serial.printf("Server listening on %s.local\n", MASTER_SERVERNAME);

#endif

    return noError;
}

ErrorCode sendCalibrationValues(String calibrationValues)
{
    HTTPClient http;
    int httpResponseCode = -1;
    int retry = 0;

    String url = String("http://") + String(MASTER_SERVERNAME) + ".local/calibration";
    Serial.printf("Sending calibration data %s\n", url.c_str());
    http.setConnectTimeout(10000);

    while (httpResponseCode < 0 && retry < MAXRETRY)
    {
        if (retry > 0)
        {
            Serial.println("Retrying...");
        }
        http.end();
        http.begin(url);
        http.addHeader("Content-Type", "text/plain");

        httpResponseCode = http.POST(calibrationValues);

        retry++;
    }
    if (httpResponseCode < 0)
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
        return cannotSendCalibrationValues;
    }
    return noError;
}

int HTTPGetWithRetry(HTTPClient *http, String url, int httpCode)
{
    int retry = 0;
    httpCode = http->GET();
    while (httpCode != HTTP_CODE_OK && retry < MAXRETRY)
    {
        Serial.printf("Calling %s\n", url.c_str());
        if (retry > 0)
        {
            Serial.printf("Received %s, Retrying...\n", http->errorToString(httpCode).c_str());
        }
        http->end();
        http->begin(url);
        httpCode = http->GET();
        retry++;
        delay(PAUSEDELAYRETRY);
    }
    return httpCode;
}

// Call to master to see if it is ready to receive calibrated data
bool isMasterReady()
{
    HTTPClient http;
    int httpCode;
    bool ready = false;

    String url = String("http://") + String(MASTER_SERVERNAME) + ".local/ready";
    http.setConnectTimeout(10000);
    httpCode = HTTPGetWithRetry(&http, url, httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        ready = (payload == "true");

        Serial.printf("[HTTP] GET succeeded, masterReady: %s\n", payload.c_str());
        return ready;
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        lastError = cannotGetIfMasterIsReady;
        return false;
    }
}

uint8_t *getGifData(String esp_id, int screenNumber, size_t *bufferSize)
{
    HTTPClient http;
    int httpCode;

    String url = apiEndpoint + apiGif + esp_id + "/" + String(screenNumber);
    void *gifData = NULL;
    // http.setConnectTimeout(10000);
    httpCode = HTTPGetWithRetry(&http, url, httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
        size_t contentLength = http.getSize();
        // Allocate memory with posix_memalign
        int result = posix_memalign(&gifData, 8, contentLength);
        if (result != 0)
        {
            Serial.println("Error: Memory allocation for getGifData failed.");
            lastError = notEnoughMemory;
            return NULL;
        }
        *bufferSize = contentLength;

        WiFiClient *stream = http.getStreamPtr();
        size_t totalBytesRead = 0;

        while (http.connected() && (contentLength > 0 || contentLength == -1))
        {
            size_t availableBytes = stream->available();
            if (availableBytes > 0)
            {
                int bytesRead = stream->readBytes((uint8_t *)gifData + totalBytesRead, availableBytes);
                totalBytesRead += bytesRead;
                contentLength -= bytesRead;
            }
            // yield();
        }

        http.end();
        yield();
        // Serial.printf("Total bytes read: %lu\n", totalBytesRead);
        return (uint8_t *)gifData;
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        lastError = cannotGetGifFiles;
        return NULL;
    }
}