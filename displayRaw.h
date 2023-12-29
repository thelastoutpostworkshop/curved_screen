#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager

#define colorOutputSize 2 // 8 bit color as output
#define imageWidth 240
#define imageHeight 240

TFT_eSPI tft = TFT_eSPI(); // Make sure SPI_FREQUENCY is 20000000 in your TFT_eSPI driver for your display if on a breadboard

void initDisplay(void)
{
    tft.init();
    tft.setFreeFont(&FreeSans9pt7b);
}

class Display
{
private:
    int csPin; // Chip Select pin
    size_t imageSize;
    uint16_t currentFrame = 0;
    uint16_t frameCount = 0;
    uint8_t *frames[256];

    void activate(void)
    {
        digitalWrite(csPin, LOW);
    }
    void deActivate(void)
    {
        digitalWrite(csPin, HIGH);
    }


public:
    int chipSelectPin() const
    {
        return csPin;
    }
    uint8_t *allocateBuffer(void)
    {
        size_t bufferLength = imageWidth * imageHeight * colorOutputSize;
        frames[frameCount] = (uint8_t *)malloc(bufferLength);
        frameCount++;
        return frames[frameCount - 1];
    }
    Display(int pin) : csPin(pin)
    {
        pinMode(csPin, OUTPUT);
        activate();
        tft.setRotation(2); // Adjust orientation as needed (0-3)
        tft.fillScreen(TFT_BLACK);
        deActivate();
    }
    uint16_t getFrameCount(void)
    {
        return frameCount;
    }
    uint8_t *getBuffer(int frame)
    {
        return frames[frame];
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
};