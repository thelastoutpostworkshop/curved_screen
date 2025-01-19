#include "display.h"
#include "fonts/Prototype20pt7b.h"

// Define the TFT display object
TFT_eSPI tft = TFT_eSPI();

// Main function to show a frame on a screen
void GIFDraw(GIFDRAW *pDraw)
{
    if (pDraw->y == 0)
    {
        tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
    }
    tft.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth);
}

// Initialize the TFT display
void initTFT_eSPI(void)
{
    tft.init();
    tft.setFreeFont(&Prototype20pt7b);
}

// Calibration class methods
Calibration::Calibration()
{
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        calibrationValues[i] = 0;
    }
    frameCount = 0;
}

bool Calibration::setCalibration(int frameNumber, unsigned long value)
{
    if (frameNumber >= MAX_FRAMES)
    {
        Serial.printf("Error:setCalibration frameNumber %d wrong\n", frameNumber);
        return false;
    }
    if (calibrationValues[frameNumber] < value)
    {
        calibrationValues[frameNumber] = value;
    }
    return true;
}

void Calibration::startCalibration(void)
{
    frameCount = 0;
}

int Calibration::getFrameCount(void)
{
    return frameCount;
}

unsigned long Calibration::getFrameCalibration(int frameNumber)
{
    if (frameNumber >= MAX_FRAMES)
    {
        Serial.printf("Error:getFrameCalibration frameNumber %d wrong\n", frameNumber);
        return ULONG_MAX;
    }
    return calibrationValues[frameNumber];
}

bool Calibration::addCalibration(unsigned long value)
{
    if (frameCount >= MAX_FRAMES)
    {
        Serial.println("Error: Maximum frame count reached.");
        return false;
    }
    if (calibrationValues[frameCount] < value)
    {
        calibrationValues[frameCount] = value;
    }
    frameCount++;
    return true;
}

String Calibration::getCalibrationValues(void)
{
    String payload = "";
    for (int frameNumber = 0; frameNumber < frameCount; frameNumber++)
    {
        if (frameNumber > 0)
        {
            payload += ";";
        }
        payload += String(frameNumber) + "," + String(calibrationValues[frameNumber]);
    }
    return payload;
}

bool Calibration::retrieveCalibrationValues(String calibrationReceived)
{
    int frameStart = 0;
    int nextFrame;
    while ((nextFrame = calibrationReceived.indexOf(';', frameStart)) != -1)
    {
        String frameData = calibrationReceived.substring(frameStart, nextFrame);
        int commaIndex = frameData.indexOf(',');
        int frameNumber = frameData.substring(0, commaIndex).toInt();
        unsigned long calibrationValue = frameData.substring(commaIndex + 1).toInt();
        if (!setCalibration(frameNumber, calibrationValue))
        {
            return false;
        }
        frameStart = nextFrame + 1;
    }

    String lastFrameData = calibrationReceived.substring(frameStart);
    if (lastFrameData.length() > 0)
    {
        int commaIndex = lastFrameData.indexOf(',');
        int frameNumber = lastFrameData.substring(0, commaIndex).toInt();
        unsigned long calibrationValue = lastFrameData.substring(commaIndex + 1).toInt();
        if (!setCalibration(frameNumber, calibrationValue))
        {
            return false;
        }
    }
    return true;
}

// Display Class Implementation

Display::Display(int pin, int rotation) 
    : csPin(pin), screenRotation(rotation), gifData(nullptr), gifSize(0), pTurboGIFBuffer(nullptr), frameGIFBuffer(nullptr)
{
    pinMode(csPin, OUTPUT);
    activate();
    tft.fillScreen(TFT_BLACK);
    deActivate();
}

void Display::activate(void)
{
    digitalWrite(csPin, LOW);
    tft.setRotation(screenRotation);
}

void Display::deActivate(void)
{
    digitalWrite(csPin, HIGH);
}

int Display::chipSelectPin() const
{
    return csPin;
}

void Display::addGif(uint8_t *gifFile, size_t gifLength)
{
    frameCount = 0;
    gifData = gifFile;
    gifSize = gifLength;
}

int Display::openGif(void)
{
    gif.begin(GIF_PALETTE_RGB565_BE);

    // Allocate memory for turbo buffer
    pTurboGIFBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (imageWidth * imageHeight), MALLOC_CAP_8BIT);
    if (pTurboGIFBuffer == nullptr)
    {
        Serial.println("Could not allocate pTurboBuffer");
        return 0;
    }

    // Allocate memory for frame buffer
    frameGIFBuffer = (uint8_t *)malloc(imageWidth * imageHeight * 2);
    if (frameGIFBuffer == nullptr)
    {
        Serial.println("Could not allocate frameBuffer");
        return 0;
    }

    gif.setDrawType(GIF_DRAW_COOKED);
    gif.setTurboBuf(pTurboGIFBuffer);
    gif.setFrameBuf(frameGIFBuffer);

    return gif.open(gifData, gifSize, GIFDraw);
}

uint16_t Display::getFrameCount(void)
{
    return frameCount;
}

void Display::clearScreen(void)
{
    activate();
    tft.fillScreen(TFT_BLACK);
    deActivate();
}

void Display::showText(const char *text, int16_t line, uint16_t color)
{
    activate();
    tft.setCursor(10, line);
    tft.setTextColor(color);
    tft.println(text);
    deActivate();
}

// Destructor to free allocated memory
Display::~Display()
{
    if (pTurboGIFBuffer)
    {
        heap_caps_free(pTurboGIFBuffer);
        pTurboGIFBuffer = nullptr;
    }

    if (frameGIFBuffer)
    {
        free(frameGIFBuffer);
        frameGIFBuffer = nullptr;
    }
}
