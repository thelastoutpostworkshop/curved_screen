
#include "displayRaw.h"
#include "webserver.h"

#define FRAME_BUFFER_SIZE 50000L
uint8_t *frameBuffer;
int framesCount;

typedef struct
{
    int csPin;
    int rotation;
    Display *display;
} Screen;

#define SCREEN_COUNT 4

Screen grid[SCREEN_COUNT] = {
    {.csPin = 6, .rotation = 0},
    {.csPin = 7, .rotation = 0},
    {.csPin = 16, .rotation = 2},
    {.csPin = 15, .rotation = 2}};

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

bool getJPGFrames(void)
{

    String frameText = "";
    framesCount = getFramesCount();
    Serial.printf("Frames Count = %d\n", framesCount);

    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Screen currentScreen = grid[i];
        for (int frameIndex = 0; frameIndex < framesCount; frameIndex++)
        {
            size_t jpgsize = getFrameJPGData(i, frameIndex, frameBuffer, FRAME_BUFFER_SIZE);
            if (jpgsize == 0)
            {
                return false;
            }
            currentScreen.display->addNewFrame(frameBuffer, jpgsize);
            frameText = String(frameIndex + 1) + "/" + String(framesCount);
            grid[i].display->clearScreen();
            grid[i].display->showCenteredText("Getting Frames",50,TFT_GREEN);
            grid[i].display->showCenteredText(frameText.c_str(), 100,TFT_GREEN);
            yield();
        }
    }
    return true;
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

void displayErrorMessage(char *message)
{
    grid[0].display->showText(message, TFT_ORANGE);
}
void displayNormalMessage(const char *message)
{
    grid[0].display->showText(message, TFT_GREEN);
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
        displayErrorMessage("No Memory for Frame Buffer");
        while (true)
            ;
    }

    if (!getJPGFrames())
    {
        Serial.println("Error: Could not retrieved all the jpg images, cannot continue.");
        displayErrorMessage("Could not retrieved all the jpg images");
        while (true)
            ;
    }

    Serial.printf("PSRAM left = %lu\n", formatBytes(ESP.getFreePsram()));
    String psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    displayNormalMessage(psram.c_str());
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
