
#include "displayRaw.h"
#include "webserver.h"

#define FRAME_BUFFER_SIZE 50000L
uint8_t *frameBuffer;

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

    int framesCount = getFramesCount();
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
            yield();
        }
    }
    return true;
}

void displayErrorMessage(char *message) {
    grid[0].display->showText(message,TFT_ORANGE);
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
        while (true)
            ;
    }

    if (!getJPGFrames())
    {
        Serial.println("Error: Could not retrieved all the jpg images, cannot continue.");
        while (true)
            ;
    }

    Serial.printf("PSRAM left = %lu\n", ESP.getFreePsram());
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
