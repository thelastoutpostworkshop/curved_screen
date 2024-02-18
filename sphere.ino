
#include "JPEGDEC.h"
#include "displayRaw.h"
#include "webserver.h"

uint8_t *fileBuffer = NULL;
size_t fileBufferSize = 0;

JPEGDEC jpeg;

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

int frameCount;

void setup()
{
    Serial.begin(115200);
    initWebServer();
    initTFT_eSPI();
    createDisplay();

    // getFrames();

    frameCount = getFramesCount();
    Serial.printf("Frames Count = %d\n", frameCount);
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->addNewFrame();
    }
    Serial.printf("PSRAM left = %lu\n", ESP.getFreePsram());
}

int draw(JPEGDRAW *pDraw)
{
    tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
    return 1;
}

int currentFrame = 0;
unsigned long t;

void loop()
{
    // for (int i = 0; i < SCREEN_COUNT; i++)
    // {
    //     grid[i].display->showFrames();
    // }

    t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Serial.printf("Getting JPG Screen %d\n",i);
        grid[i].display->imageSize = getFrameJPGData(i, currentFrame, grid[i].display->getFrame(0), grid[i].display->getFrameSize());
        currentFrame++;
        if (currentFrame == frameCount)
        {
            currentFrame = 0;
        }
    }

    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Serial.printf("Writing JPG Screen %d\n",i);
        jpeg.openRAM(grid[i].display->getFrame(0), grid[i].display->imageSize, draw);
        grid[i].display->activate();
        tft.setRotation(grid[i].display->screenRotation);
        while (jpeg.decode(0,0,0))
        {
            /* code */
        }
        grid[i].display->deActivate();
        jpeg.close();
        
    }
    Serial.printf("Took %ld ms\n", millis() - t);

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
