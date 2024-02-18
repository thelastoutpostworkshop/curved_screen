#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager

#define colorOutputSize 2 // 8 bit color as output
#define imageWidth 240
#define imageHeight 240

TFT_eSPI tft = TFT_eSPI(); // Make sure SPI_FREQUENCY is 20000000 in your TFT_eSPI driver for your display if on a breadboard

void initTFT_eSPI(void)
{
    tft.init();
    // tft.setSwapBytes(true);
    tft.setFreeFont(&FreeSans9pt7b);
}

#define MAX_FRAMES 256
class Display
{
private:
    int csPin; // Chip Select pin
    int screenRotation;
    uint16_t currentFrame = 0;
    uint16_t frameCount = 0;
    uint8_t *frames[MAX_FRAMES];

public:
    size_t imageSize;
    void activate(void)
    {
        digitalWrite(csPin, LOW);
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
        if (frameCount >= MAX_FRAMES) // Assuming MAX_FRAMES is defined as the maximum number of frames
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

    Display(int pin,int rotation) : csPin(pin),screenRotation(rotation)
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
            tft.setRotation(screenRotation);
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
            tft.setRotation(screenRotation);
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