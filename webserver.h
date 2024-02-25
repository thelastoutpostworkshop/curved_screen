#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "sphere.h"

#ifdef MASTER
#include <ESPAsyncWebServer.h>
AsyncWebServer masterServer(80);
int slavesReady = 0;

void handleReady(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "ok");
    slavesReady++;
}
#endif

HTTPClient http;

const String apiEndpoint = "http://192.168.1.90:3000/api/";
const String apiFrameCount = "frames-count";
const String apiFrameJPG = "framejpg/";

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
    if (!MDNS.begin("sphere"))
    {
        Serial.println("Error starting mDNS for the Master");
        return noMDNS;
    }
    masterServer.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
                    { handleReady(request); });
    masterServer.begin();

#endif

    http.setReuse(true);
    return noError;
}

void sendReady(void)
{
    // String url = apiEndpoint + apiFrameJPG + esp_id + "/" + String(screenNumber) + "/" + String(frameNumber);
    // Serial.printf("Calling %s\n", url.c_str());
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
    Serial.printf("Calling %s\n", url.c_str());

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
