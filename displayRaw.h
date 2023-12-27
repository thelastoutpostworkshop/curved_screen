#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager

#define colorOutputSize 2 // 8 bit color as output
#define imageWidth 240
#define imageHeigth 240

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

    int chipSelectPin() const
    {
        return csPin;
    }

public:
    uint8_t *allocateBuffer(void)
    {
        size_t bufferLength = imageWidth * imageHeigth * colorOutputSize;
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

    void showFrame(int frame)
    {
        activate();
        tft.pushImage(0, 0, imageWidth, imageHeigth, (uint16_t *)frames[frame]);
        deActivate();
    }
    void showFrames(void)
    {
        activate();
        tft.pushImage(0, 0, imageWidth, imageHeigth, (uint16_t *)frames[currentFrame]);
        deActivate();
        currentFrame++;
        if (currentFrame == frameCount)
        {
            currentFrame = 0;
        }
    }
};