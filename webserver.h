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

int getFramesCount() {
  HTTPClient http;

  String url = apiEndpoint+"frames-count";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    http.end(); // Free resources
    return payload.toInt(); // Convert payload to integer and return
  } else {
    Serial.println("Error on HTTP request");
    http.end(); // Free resources
    return -1; // Indicate failure
  }
}