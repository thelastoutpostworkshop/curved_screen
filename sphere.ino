#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"
#include "displayRaw.h"
#include "gifdec.h"
#include "images/x_wing.h"
#include "images/x_winglarge.h"
#include "images/colortest.h"
#include "images/hyperspace.h"
#include "images/darthvader.h"
#include "images/hud_1.h"
#include "images/bb8.h"
#include "images/quiz.h"
#include "images/eyes.h"

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
        {.row = 0, .column = 1, .csPin = 15} // Column 1
    }};

int totalWidth = imageWidth * COLUMNS;
int totalHeight = imageHeight * ROWS;

const int NUM_DISPLAYS = 2; // Adjust this value based on the number of displays
// gif_load gifToLoad[NUM_DISPLAYS] = {{15, bb8, sizeof(bb8)}, {7, x_wing, sizeof(x_wing)}};
Display *display[NUM_DISPLAYS];
int displayCS[NUM_DISPLAYS] = {15, 7};

void setup()
{
    Serial.begin(115200);
    initWebServer();
    if (!psramFound())
    {
        Serial.println("!!! No PSRAM detected, cannot continue");
        return;
    }
    else
    {
        Serial.printf("PSRAM Size=%ld\n", ESP.getPsramSize());
    }
    initDisplay();

    for (int i = 0; i < NUM_DISPLAYS; i++)
    {
        display[i] = new Display(displayCS[i]);
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLUMNS; c++)
        {
            grid[r][c].display = new Display(grid[r][c].csPin);
        }
    }

    gif = new GIF();
}

void loop()
{
    if (imageReadytoDisplay)
    {
        for (int r = 0; r < ROWS; r++)
        {
            for (int c = 0; c < COLUMNS; c++)
            {
                grid[r][c].display->showFrames();
            }
        }
    }
}

void processGifImage(uint8_t *fileBuffer, size_t fileBufferSize)
{
    gif->gd_open_gif_memory(fileBuffer, fileBufferSize, colorOutputSize);
    Serial.printf("Width=%u, Height=%u\n", gif->info()->width, gif->info()->height);
    if (gif->info()->width != totalWidth && gif->info()->height != totalWidth)
    {
        Serial.printf("GIF don't fit in the screen arrangement w=%d, h=%d\n", totalWidth, totalWidth);
        return;
    }
    uint8_t *buffer;
    size_t bufferLength = gif->info()->width * gif->info()->height * colorOutputSize;
    buffer = (uint8_t *)malloc(bufferLength);
    if (buffer == NULL)
    {
        Serial.println("Not enough memory");
        return;
    }

    Serial.println("Extracting frames...");
    int frameCount = 0;
    while (gif->gd_get_frame())
    {
        gif->gd_render_frame(buffer);

        for (int r = 0; r < ROWS; r++)
        {
            for (int c = 0; c < COLUMNS; c++)
            {
                grid[r][c].display->allocateBuffer();
                if (grid[r][c].display == NULL)
                {
                    Serial.printf("Not Enough memory for frames, frames rendered is %u\n", grid[r][c].display->getFrameCount());
                    return;
                }
                getScreenImage(buffer, grid[r][c], frameCount);
            }
        }
        frameCount++;
    }

    free(buffer);
    Serial.printf("Frames = %u\n", frameCount);
    Serial.print("Free PSRAM: ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" bytes");

    imageReadytoDisplay = true;
}

void getScreenImage(const uint8_t *originalBuffer, Screen screen, int frame)
{
    int sourceX = screen.column * imageWidth;
    int sourceY = screen.row * imageHeight;

    for (int row = 0; row < imageHeight; ++row)
    {
        memcpy(screen.display->getBuffer(frame) + row * imageWidth * colorOutputSize,
               originalBuffer + ((sourceY + row) * totalWidth + sourceX) * colorOutputSize,
               imageWidth * colorOutputSize);
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
                imageReadytoDisplay=false; 
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
