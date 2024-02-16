#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"
#include "displayRaw.h"

AsyncWebServer server(80);

uint8_t *fileBuffer = NULL;
size_t fileBufferSize = 0;

bool imageReadytoDisplay = false;

GIF *gif;

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

int totalWidth = imageWidth * COLUMNS;
int totalHeight = imageHeight * ROWS;

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

    server.on(
        "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
        {
        request->send(200, "text/plain", "File Uploaded");
        if (fileBuffer) {
            free(fileBuffer); // Free the buffer after processing
            fileBuffer = nullptr;
            fileBufferSize = 0;
        } },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            if (index == 0)
            {
                Serial.printf("Upload Start: %s\n", filename.c_str());
                imageReadytoDisplay = false;
            }

            // Resize buffer if necessary
            if (fileBufferSize + len > fileBufferSize)
            {
                uint8_t *newBuffer = (uint8_t *)realloc(fileBuffer, fileBufferSize + len);
                if (!newBuffer)
                {
                    Serial.println("Failed to allocate memory");
                    return;
                }
                fileBuffer = newBuffer;
            }

            // Append data to buffer
            memcpy(fileBuffer + fileBufferSize, data, len);
            fileBufferSize += len;

            if (final)
            {
                Serial.printf("Upload Complete: %s, size: %u\n", filename.c_str(), fileBufferSize);
                processGifImage(fileBuffer, fileBufferSize);
            }
        });
    server.begin();
}
