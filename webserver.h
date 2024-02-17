#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    Serial.printf("Data received: %lu bytes\n", len);
    // Add more debugging here if needed
}

void imageReceive(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        handleWebSocketMessage(arg, data, len);
    }
    // Handle other events like WS_EVT_CONNECT, WS_EVT_DISCONNECT, etc.
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

    ws.onEvent(imageReceive);
    server.addHandler(&ws);

    server.begin();
}