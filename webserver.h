#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include "secrets.h"

AsyncWebServer server(80);

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

    HTTPClient http;
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
    HTTPClient http;

    if (!buffer)
    {
        return -1; // Invalid buffer
    }

    // Ensure variables are converted to String for URL construction
    String url = apiEndpoint + "frame/" + String(screenNumber) + "/" + String(frameNumber);
    // Serial.printf("Calling %s\n", url.c_str());

    http.setReuse(true);
    http.begin(url);           // Start the connection
    int httpCode = http.GET(); // Make the GET request

    if (httpCode == HTTP_CODE_OK)
    {
        WiFiClient *stream = http.getStreamPtr();
        size_t totalBytesRead = 0;
        // Loop to read bytes as long as there's data available and buffer has space
        while (totalBytesRead < bufferSize)
        {
            size_t availableBytes = stream->available();
            if (availableBytes > 0)
            {
                int bytesRead = stream->readBytes(buffer + totalBytesRead, min(availableBytes, bufferSize - totalBytesRead));
                totalBytesRead += bytesRead;
            }
            else if (!http.connected())
            {
                break; // Break if no longer connected
            }
        }
        http.end(); // End connection
        // Serial.printf("Total bytes read: %lu\n", totalBytesRead);
        return totalBytesRead; // Return total bytes read
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end(); // Ensure connection is closed
        return -2;  // HTTP error or no data read
    }
}

#include <HTTPClient.h>

size_t getFrameJPGData(int screenNumber, int frameNumber, uint8_t *buffer, size_t bufferSize)
{
    if (!buffer)
    {
        return -1; // Invalid buffer
    }

    HTTPClient http;
    String url = apiEndpoint + "framejpg/" + String(screenNumber) + "/" + String(frameNumber);
    // Serial.printf("Calling %s\n", url.c_str());

    http.setReuse(true);
    http.begin(url);           // Start the connection
    int httpCode = http.GET(); // Make the GET request

    if (httpCode == HTTP_CODE_OK)
    {
        size_t contentLength = http.getSize(); // Get the size of the response payload
        WiFiClient *stream = http.getStreamPtr();
        size_t totalBytesRead = 0;
        while (totalBytesRead < bufferSize && totalBytesRead < contentLength)
        {
            size_t availableBytes = stream->available();
            if (availableBytes > 0)
            {
                int bytesRead = stream->readBytes(buffer + totalBytesRead, min(min(availableBytes, bufferSize - totalBytesRead), contentLength - totalBytesRead));
                totalBytesRead += bytesRead;
            }
            else if (!http.connected())
            {
                break; 
            }
        }
        http.end(); 
        // Serial.printf("Total bytes read: %lu\n", totalBytesRead);
        return totalBytesRead; // Return total bytes read
    }
    else
    {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end(); 
        return -2;  
    }
}
