
#include "displayRaw.h"
#include "wear_levelling.h"
#include "frame_001.h"
#include "webserver.h"

uint8_t *fileBuffer = NULL;
size_t fileBufferSize = 0;

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
    {.csPin = 15, .rotation = 2},
    {.csPin = 16, .rotation = 2}};

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

void getFrames(void)
{
    int framesCount = getFramesCount();
    Serial.printf("Frames Count = %d\n", framesCount);

    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Screen currentScreen = grid[i];
        for (int frameIndex = 0; frameIndex < framesCount; frameIndex++)
        {
            u_int8_t *frame = currentScreen.display->addNewFrame();
            if (frame != NULL)
            {
                getFrameData(i, frameIndex, frame, currentScreen.display->getFrameSize());
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    initWebServer();
    initTFT_eSPI();
    createDisplay();

    getFrames();
}

void loop()
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
            grid[i].display->showFrames();
    }
}
