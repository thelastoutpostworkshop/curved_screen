
#include "displayRaw.h"
#include "webserver.h"

#define FRAME_BUFFER_SIZE 50000L
uint8_t *frameBuffer;
int framesCount;
uint64_t esp_id;

typedef struct
{
    int csPin;
    int rotation;
    Display *display;
} Screen;

enum ErrorCode
{
    noError,
    noFrames,
    cannotGetJPGFrames
};

#define SCREEN_COUNT 4

Screen grid[SCREEN_COUNT] = {
    {.csPin = 3, .rotation = 0},
    {.csPin = 8, .rotation = 0},
    {.csPin = 18, .rotation = 2},
    {.csPin = 17, .rotation = 2}};

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

ErrorCode getJPGFrames(void)
{
    String frameText = "";
    framesCount = getFramesCount();
    Serial.printf("Frames Count = %d\n", framesCount);

    if (framesCount <= 0)
    {
        return noFrames;
    }

    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Screen currentScreen = grid[i];
        for (int frameIndex = 0; frameIndex < framesCount; frameIndex++)
        {
            size_t jpgsize = getFrameJPGData(i, frameIndex, frameBuffer, FRAME_BUFFER_SIZE);
            if (jpgsize == 0)
            {
                return cannotGetJPGFrames;
            }
            currentScreen.display->addNewFrame(frameBuffer, jpgsize);
            frameText = String(frameIndex + 1) + "/" + String(framesCount);
            grid[i].display->clearScreen();
            grid[i].display->showText("Frames", 50, TFT_GREEN);
            grid[i].display->showText(frameText.c_str(), 100, TFT_GREEN);
            yield();
        }
    }
    return noError;
}

String formatBytes(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(bytes) + " B";
    }
    else if (bytes < (1024 * 1024))
    {
        return String(bytes / 1024.0, 2) + " KB";
    }
    else
    {
        return String(bytes / 1024.0 / 1024.0, 2) + " MB";
    }
}

void displayErrorMessage(char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_ORANGE);
}
void displayNormalMessage(const char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_GREEN);
}

void setup()
{
    Serial.begin(115200);
    initWebServer();
    initTFT_eSPI();
    createDisplay();

    frameBuffer = (uint8_t *)malloc(FRAME_BUFFER_SIZE);

    if (frameBuffer == NULL)
    {
        Serial.println("Error: Memory allocation failed for frame buffer, cannot continue.");
        displayErrorMessage("No Memory for Frame Buffer", 40);
        while (true)
            ;
    }

    // Retrieve all the JPG Frames
    ErrorCode res = getJPGFrames();
    if (res != noError)
    {
        switch (res)
        {
        case noFrames:
            Serial.println("Error: No Frames.");
            displayErrorMessage("No Frames", 40);
            break;

        case cannotGetJPGFrames:
            Serial.println("Error: Could not retrieved all the jpg images, cannot continue.");
            displayErrorMessage("Could not retrieved all the jpg images", 40);
            break;
        }
        while (true)
            ;
    }

    // Show mac number for identification by the server
    esp_id = ESP.getEfuseMac();
    String idmsg = String(esp_id);
    Serial.printf("id=%s\n", idmsg.c_str());

    Serial.printf("PSRAM left = %lu\n", formatBytes(ESP.getFreePsram()));
    String psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    displayNormalMessage(psram.c_str(), 40);
    delay(5000);
}

unsigned long t;
void loop()
{
    // t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->showJPGFrames();
    }
    // Serial.printf("Took %ld ms\n", millis() - t);
}
