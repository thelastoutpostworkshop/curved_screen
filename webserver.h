#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include "sphere.h"

WiFiUDP udp;
unsigned int localUdpPort = 4210;                 // Local port to listen on
char incomingPacket[255];                         // Buffer for incoming packets
const char *broadcastAddress = "255.255.255.255"; // Broadcast address

#ifdef MASTER
AsyncWebServer masterServer(80);
int slavesReady = 0;

void handleReady(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "ok");
    slavesReady++;
}
void waitForSlavesToshowFrame(void)
{
    slavesReady = 0;
    while (slavesReady < SLAVECOUNT)
    {
        int packetSize = udp.parsePacket();
        if (packetSize)
        {
            // Read the packet into the buffer
            int len = udp.read(incomingPacket, 255);
            if (len > 0)
            {
                incomingPacket[len] = 0; // Null-terminate the string
                if (strcmp(incomingPacket, "ready") == 0)
                {
                    slavesReady++;
                    Serial.println("Slave ready received");
                }
            }
        }
    }
}
#endif

HTTPClient http;

const String apiEndpoint = "http://192.168.1.90:3000/api/";
const String apiFrameCount = "frames-count";
const String apiFrameJPG = "framejpg/";

bool waitForCommand(const char *command)
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        // read the packet into the buffer
        int len = udp.read(incomingPacket, sizeof(incomingPacket));
        if (len > 0)
        {
            incomingPacket[len] = 0; // Null-terminate the string
        }
        // Serial.printf("Received: %s\n", incomingPacket);
        if (strcmp(incomingPacket, command) == 0)
        {
            return true; // Command matches the expected command
        }
    }
    return false; // No matching command received
}

void broadcastCommand(const char *command)
{
    udp.beginPacket(broadcastAddress, localUdpPort);
    udp.write((uint8_t *)command, strlen(command)); // Correctly send the command as bytes
    if (udp.endPacket())
    {
        // Serial.printf("Command \"%s\" broadcasted successfully.\n", command);
    }
    else
    {
        Serial.println("Error broadcasting command.");
    }
}

void sendMessageToServer(const char *message)
{
    String url = String("http://") + String(SERVERNAME) + ".local";
    udp.beginPacket(url.c_str(), localUdpPort);
    udp.write((uint8_t *)message, strlen(message));
    udp.endPacket();
}

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

    udp.begin(localUdpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
#ifdef MASTER
    if (!MDNS.begin(SERVERNAME))
    {
        Serial.println("Error starting mDNS for the Master");
        return noMDNS;
    }
    masterServer.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
                    { handleReady(request); });
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
