#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "sphere.h"

#define MAXRETRY 10
#define PAUSEDELAYRETRY 100

#ifdef MASTER
AsyncWebServer masterServer(80);

const char *homePage = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Home Page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>Welcome to the ESP32 Home Page!</h1>
  <p>This is a simple example of serving an HTML page from the ESP32.</p>
</body>
</html>
)rawliteral";

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
#endif

const String apiEndpoint = "http://192.168.1.90:3000/api/";
const String apiFrameCount = "frames-count";
const String apiFrameJPG = "framejpg/";
const String apiGif = "gif/";

ErrorCode initWebServer()
{
    Serial.println("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

#ifdef MASTER
    if (!MDNS.begin(SERVERNAME))
    {
        Serial.println("Error starting mDNS for the Master");
        return noMDNS;
    }
    // masterServer.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
    //                 { handleReady(request); });
    masterServer.on(
        "/calibration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            handleCalibrationData(data, len, request);
        });
    masterServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", homePage); });
    masterServer.begin();
    Serial.printf("Server listening on %s.local\n", SERVERNAME);

#endif

    return noError;
}

ErrorCode sendCalibrationValues(String calibrationValues)
{
    HTTPClient http;
    int httpResponseCode = -1;
    int retry = 0;

    String url = String("http://") + String(SERVERNAME) + ".local/calibration";
    Serial.printf("Sending calibration data %s\n", url.c_str());

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

        // if (httpResponseCode > 0)
        // {
        //     String response = http.getString(); // Get server response
        //     Serial.println(httpResponseCode);
        //     Serial.println(response);
        // }
        retry++;
    }
    if (httpResponseCode < 0)
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
        return cannotSendCalibrationValues;
    }
    return noError;
}

void sendReady(void)
{
    HTTPClient http;

    String url = String("http://") + String(SERVERNAME) + ".local/ready";
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("Sending ready %s\n", url.c_str());
    if (httpCode < 0)
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

int getFramesCount()
{
    HTTPClient http;

    String url = apiEndpoint + apiFrameCount;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        String payload = http.getString();
        http.end();
        return payload.toInt();
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return -1;
    }
}

size_t getFrameJPGData(String esp_id, int screenNumber, int frameNumber, uint8_t *buffer, size_t bufferSize)
{
    HTTPClient http;

    String url = apiEndpoint + apiFrameJPG + esp_id + "/" + String(screenNumber) + "/" + String(frameNumber);
    // Serial.printf("Calling %s\n", url.c_str());

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        size_t contentLength = http.getSize();
        if (contentLength > bufferSize)
        {
            Serial.printf("getFrameJPGData content too large, content size = %lu, bufferSize=%lu\n", contentLength, bufferSize);
            return 0;
        }
        WiFiClient *stream = http.getStreamPtr();
        size_t totalBytesRead = 0;

        while (http.connected() && (contentLength > 0 || contentLength == -1))
        {
            size_t availableBytes = stream->available();
            if (availableBytes > 0)
            {
                int bytesRead = stream->readBytes(buffer + totalBytesRead, availableBytes);
                totalBytesRead += bytesRead;
                contentLength -= bytesRead;
            }
            yield();
        }

        http.end();
        yield();
        // Serial.printf("Total bytes read: %lu\n", totalBytesRead);
        return totalBytesRead;
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return 0;
    }
}

int HTTPGetWithRetry(HTTPClient *http, String url, int httpCode)
{
    int retry = 0;
    while (httpCode != HTTP_CODE_OK && retry < MAXRETRY)
    {
        Serial.printf("Calling %s\n", url.c_str());
        if (retry > 0)
        {
            Serial.println("Retrying...");
        }
        http->end();
        http->begin(url);
        httpCode = http->GET();
        retry++;
        delay(PAUSEDELAYRETRY);
    }
    return httpCode;
}

uint8_t *getGifData(String esp_id, int screenNumber, size_t *bufferSize)
{
    HTTPClient http;
    int httpCode;

    String url = apiEndpoint + apiGif + esp_id + "/" + String(screenNumber);
    void *gifData = NULL;

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
            yield();
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
