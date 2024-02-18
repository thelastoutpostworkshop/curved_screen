#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include "secrets.h"

AsyncWebServer server(80);
HTTPClient http;

const String apiEndpoint = "http://192.168.1.90:3000/api/";

void initWebServer()
{
    Serial.println("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    if (!MDNS.begin("sphere"))
    {
        Serial.println("Error starting mDNS");
        return;
    }

    server.begin();
}

int getFramesCount()
{

    String url = apiEndpoint + "frames-count";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        String payload = http.getString();
        http.end();             // Free resources
        return payload.toInt(); // Convert payload to integer and return
    }
    else
    {
        Serial.println("Error on HTTP request");
        http.end(); // Free resources
        return -1;  // Indicate failure
    }
}

int getFrameData(int screenNumber, int frameNumber, uint8_t *buffer, size_t bufferSize)
{
    if (!buffer)
    {
        return -1; // Invalid buffer
    }

    const String url = apiEndpoint + "frame/" + screenNumber + "/" + frameNumber;
    Serial.printf("Calling %s\n", url.c_str());

    http.begin(url);           // Start the connection
    int httpCode = http.GET(); // Make the GET request

    if (httpCode > 0)
    { // Check for the returning code
        if (httpCode == HTTP_CODE_OK)
        {
            WiFiClient *stream = http.getStreamPtr();
            size_t totalBytesRead = 0;
            while (http.connected() && (totalBytesRead < bufferSize))
            {
                // Available bytes to read
                size_t availableBytes = stream->available();
                if (availableBytes)
                {
                    int bytesRead = stream->readBytes(buffer + totalBytesRead, min(availableBytes, bufferSize - totalBytesRead));
                    totalBytesRead += bytesRead;
                    if (totalBytesRead >= bufferSize)
                    {
                        Serial.printf("Buffer is full %lu, remaining to read %lu\n", totalBytesRead,bytesRead);
                        break; // Buffer is full
                    }
                }
                delay(10); // Small delay to allow data to arrive
            }
            http.end(); // End connection
            return totalBytesRead;
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end(); // End connection
    return -2;  // HTTP error or no data read
}