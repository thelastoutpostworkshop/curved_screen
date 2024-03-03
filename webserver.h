#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "sphere.h"

#ifdef MASTER
AsyncWebServer masterServer(80);

void handleReady(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "ok");
    slaves->addSlavesReady();
}

// Function to process the calibration data
void processCalibrationData(uint8_t *data, size_t len, AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "Calibration data received");
}
#endif

HTTPClient http;

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
    masterServer.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
                    { handleReady(request); });
    masterServer.on(
        "/calibration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            processCalibrationData(data, len, request);
        });

    masterServer.begin();
    Serial.printf("Server listening on %s.local\n", SERVERNAME);

#endif

    http.setReuse(true);
    return noError;
}

void sendReady(void)
{
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
    String url = apiEndpoint + apiFrameJPG + esp_id + "/" + String(screenNumber) + "/" + String(frameNumber);
    // Serial.printf("Calling %s\n", url.c_str());

    http.setReuse(true);
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

uint8_t *getGifData(String esp_id, int screenNumber, size_t *bufferSize)
{
    String url = apiEndpoint + apiGif + esp_id + "/" + String(screenNumber);
    Serial.printf("Calling %s\n", url.c_str());

    http.setReuse(true);
    http.begin(url);
    int httpCode = http.GET();
    void *gifData = NULL;

    if (httpCode == HTTP_CODE_OK)
    {
        size_t contentLength = http.getSize();
        // Allocate memory with posix_memalign
        int result = posix_memalign(&gifData, 8, contentLength);
        if (result != 0)
        {
            Serial.println("Error: Memory allocation for getGifData failed.");
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
        return 0;
    }
}
