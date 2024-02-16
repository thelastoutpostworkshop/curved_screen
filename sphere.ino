#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"
#include "displayRaw.h"

AsyncWebServer server(80);

uint8_t *fileBuffer = NULL;
size_t fileBufferSize = 0;

typedef struct
{
    int row;
    int column;
    int csPin;
    Display *display;
} Screen;

#define ROWS 1    // Number of rows
#define COLUMNS 2 // Number of columns

Screen grid[ROWS][COLUMNS] = {
    {
        {.row = 0, .column = 0, .csPin = 7}, // Column 0
        {.row = 0, .column = 1, .csPin = 6} // Column 1
    }};

// int totalWidth = imageWidth * COLUMNS;
// int totalHeight = imageHeight * ROWS;

void setup()
{
    Serial.begin(115200);
    initWebServer();
    initDisplay();

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLUMNS; c++)
        {
            grid[r][c].display = new Display(grid[r][c].csPin);
        }
    }

}

void loop()
{

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

    server.begin();
}
