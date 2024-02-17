#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

size_t totalDateCount = 0;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    Serial.printf("Data received: %lu bytes\n", len);
    totalDateCount+=len;
    Serial.printf("Total Data received: %lu bytes\n", totalDateCount);
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

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        totalDateCount = 0;
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // Add any additional actions you want to perform when the WebSocket is opened
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        // Handle disconnection
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        // Handle incoming data
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        // Handle PONG (response to ping) and ERROR events
        break;
    }
}

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