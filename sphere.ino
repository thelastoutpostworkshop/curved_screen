
#include "displayRaw.h"
#include "webserver.h"

#define FRAME_BUFFER_SIZE 50000L
uint8_t *frameBuffer;

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
    {.csPin = 16, .rotation = 2},
    {.csPin = 15, .rotation = 2}};

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

int frameCount;

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

    // frameCount = getFramesCount();
    // Serial.printf("Frames Count = %d\n", frameCount);
    // for (int i = 0; i < SCREEN_COUNT; i++)
    // {
    //     grid[i].display->addNewFrame();
    // }
    Serial.printf("PSRAM left = %lu\n", ESP.getFreePsram());
}

int currentFrame = 0;
unsigned long t;

void loop()
{
    t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->showJPGFrames();
    }
    Serial.printf("Took %ld ms\n", millis() - t);

    // for (int i = 0; i < SCREEN_COUNT; i++)
    // {
    //     grid[i].display->imageSize = getFrameJPGData(i, currentFrame, grid[i].display->getFrame(0), grid[i].display->getFrameSize());
    //     currentFrame++;
    //     if (currentFrame == frameCount)
    //     {
    //         currentFrame = 0;
    //     }
    // }

    // for (int i = 0; i < SCREEN_COUNT; i++)
    // {
    //     jpeg.openRAM(grid[i].display->getFrame(0), grid[i].display->imageSize, draw);
    //     grid[i].display->activate();
    //     tft.setRotation(grid[i].display->screenRotation);
    //     while (jpeg.decode(0, 0, 0) == JPEG_SUCCESS)
    //     {
    //         /* code */
    //     }
    //     grid[i].display->deActivate();
    //     jpeg.close();
    // }

    // t = millis();

    // getFrameData(0, currentFrame, grid[0].display->getFrame(0), grid[0].display->getFrameSize());
    // currentFrame++;
    // if (currentFrame == frameCount)
    // {
    //     currentFrame = 0;
    // }

    // grid[0].display->showFrame(0);

    // Serial.printf("Took %ld ms\n", millis() - t);
}
