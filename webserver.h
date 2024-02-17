#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

class ESP32Server
{

private:
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
    {
        Serial.printf("Data received: %lu bytes\n", len);
    }

    void imageReceive(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
    {
        if (type == WS_EVT_DATA)
        {
            handleWebSocketMessage(arg, data, len);
        }
        // Handle other events like WS_EVT_CONNECT, WS_EVT_DISCONNECT, etc.
    }

    static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                          void *arg, uint8_t *data, size_t len)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            // Add any additional actions you want to perform when the WebSocket is opened
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            // Handle disconnection
            break;
        case WS_EVT_DATA:
            if (info->final && info->index == 0 && info->len == len)
            {
                // The whole message is in a single frame and we got all of it's data
                if (info->opcode == WS_TEXT)
                {
                    // Data is text
                    Serial.println("Text data received");
                    // Convert the data to a string to process it as text
                    String message = "";
                    for (size_t i = 0; i < len; i++)
                    {
                        message += (char)data[i];
                    }
                    Serial.println(message);
                }
                else if (info->opcode == WS_BINARY)
                {
                    // Data is binary
                    Serial.println("Binary data received");
                    // Process the binary data directly...
                }
            }
            else
            {
                Serial.println("Not final data");
            }
            break;
        case WS_EVT_PONG:
            break;
        case WS_EVT_ERROR:
            // Handle PONG (response to ping) and ERROR events
            break;
        }
    }

public:
    void initWebServer(void)
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

        ws.onEvent(onWebSocketEvent);
        server.addHandler(&ws);

        server.begin();
    }
};
