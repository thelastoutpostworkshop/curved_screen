#include "JPEGDEC.h"  // Install this library with the Arduino IDE Library Manager
#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager

#define MAX_FRAMES 256
#define colorOutputSize 2 // 16 bit color as output
#define imageWidth 240
#define imageHeight 240

#include "fonts/Prototype20pt7b.h"

TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;

void initTFT_eSPI(void)
{
    tft.init();
    tft.setSwapBytes(true);
    tft.setFreeFont(&Prototype20pt7b);
}

int draw(JPEGDRAW *pDraw)
{
    tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
    return 1;
}

class Display
{
private:
    int csPin; // Chip Select pin
    uint16_t currentFrame = 0;
    uint16_t frameCount = 0;
    uint8_t *frames[MAX_FRAMES];
    size_t framesSize[MAX_FRAMES];

public:
    int screenRotation;

    void activate(void)
    {
        digitalWrite(csPin, LOW);
        tft.setRotation(screenRotation);
    }
    void deActivate(void)
    {
        digitalWrite(csPin, HIGH);
    }
    int chipSelectPin() const
    {
        return csPin;
    }

    uint8_t *addNewFrame(void)
    {
        if (frameCount >= MAX_FRAMES)
        {
            Serial.println("Error: Maximum frame count reached.");
            return NULL;
        }

        size_t bufferLength = imageWidth * imageHeight * colorOutputSize;
        uint8_t *newFrame = (uint8_t *)malloc(bufferLength);

        if (newFrame == NULL)
        {
            Serial.println("Error: Memory allocation for new frame failed.");
            return NULL;
        }

        frames[frameCount] = newFrame;
        frameCount++;
        return newFrame;
    }

    void addNewFrame(uint8_t *frame, size_t size)
    {
        if (frameCount >= MAX_FRAMES)
        {
            Serial.println("Error: Maximum frame count reached.");
        }

        uint8_t *newFrame = (uint8_t *)malloc(size);

        if (newFrame == NULL)
        {
            Serial.println("Error: Memory allocation for new frame failed.");
        }
        else
        {
            memcpy(newFrame, frame, size);
            framesSize[frameCount] = size;
            frames[frameCount] = newFrame;
            frameCount++;
        }
    }

    Display(int pin, int rotation) : csPin(pin), screenRotation(rotation)
    {
        pinMode(csPin, OUTPUT);
        activate();
        tft.fillScreen(TFT_BLACK);
        deActivate();
    }
    uint16_t getFrameCount(void)
    {
        return frameCount;
    }
    uint8_t *getFrame(int frame)
    {
        return frames[frame];
    }
    size_t getFrameSize()
    {
        return imageWidth * imageHeight * colorOutputSize;
    }
    void freeFrames(void)
    {
        for (int i = 0; i < frameCount; i++)
        {
            free(frames[i]);
        }
        frameCount = 0;
        currentFrame = 0;
    }
    void showFrame(int frame)
    {
        if (frame < frameCount)
        {
            activate();
            tft.pushImage(0, 0, imageWidth, imageHeight, (uint16_t *)frames[frame]);
            deActivate();
        }
        else
        {
            Serial.printf("!!! No frame %d\n", frame);
        }
    }
    void showFrames(void)
    {
        if (frameCount > 0)
        {
            activate();
            tft.pushImage(0, 0, imageWidth, imageHeight, (uint16_t *)frames[currentFrame]);
            deActivate();
            currentFrame++;
            if (currentFrame == frameCount)
            {
                currentFrame = 0;
            }
        }
        else
        {
            Serial.println("!!! No frames to show");
        }
    }
    void showJPGFrames(void)
    {
        if (frameCount > 0)
        {
            int res = jpeg.openRAM(frames[currentFrame], framesSize[currentFrame], draw);
            activate();
            while (jpeg.decode(0, 0, 0) == JPEG_SUCCESS)
            {
                /* jpg file is decoded */
            }
            deActivate();
            jpeg.close();

            currentFrame++;
            if (currentFrame == frameCount)
            {
                currentFrame = 0;
            }
        }
        else
        {
            Serial.println("!!! No frames to show");
        }
    }
    void clearScreen(void)
    {
        activate();
        tft.fillScreen(TFT_BLACK);
        deActivate();
    }
    void showText(const char *text, u_int16_t color)
    {
        activate();
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(10, 40);
        tft.setTextColor(color);
        tft.println(text);
        deActivate();
    }
    void showCenteredText(const char *text, int16_t line, uint16_t color)
    {
        activate();
        tft.setFreeFont(&Prototype20pt7b);
        tft.setTextColor(color);

        int16_t textWidth = tft.textWidth(text);

        int x = (tft.width() - textWidth) / 2;

        int16_t fontHeight = tft.fontHeight();
        int y = fontHeight * line - (fontHeight / 2); // Adjust as needed for exact positioning

        tft.setCursor(x, y);
        tft.print(text);
        deActivate();
    }
};