// This file provides functions for the calibration and to show GIF

#include <TFT_eSPI.h>    // Install this library with the Arduino IDE Library Manager
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager

#define MAX_FRAMES 256   // Maximum frames that the GIF can have, used for calibration, you can increase this value if needed
#define colorOutputSize 2 // 16 bit color as output
#define imageWidth 240
#define imageHeight 240

#include "fonts/Prototype20pt7b.h"

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

void initTFT_eSPI(void)
{
  tft.init();
  // tft.setSwapBytes(true);
  tft.setFreeFont(&Prototype20pt7b);
}

// Calibration functions
class Calibration
{
private:
  unsigned long calibrationValues[MAX_FRAMES];
  int frameCount;

  bool setCalibration(int frameNumber, unsigned long value)
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

public:
  Calibration()
  {
    for (int i = 0; i < MAX_FRAMES; i++)
    {
      calibrationValues[i] = 0;
    }
    frameCount = 0;
  }
  void startCalibration(void)
  {
    frameCount = 0;
  }
  int getFrameCount(void)
  {
    return frameCount;
  }
  unsigned long getFrameCalibration(int frameNumber)
  {
    if (frameNumber >= MAX_FRAMES)
    {
      Serial.printf("Error:getFrameCalibration frameNumber %d wrong\n", frameNumber);
      return ULONG_MAX;
    }
    return calibrationValues[frameNumber];
  }
  bool addCalibration(unsigned long value)
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

  String getCalibrationValues(void)
  {
    String payload = "";
    for (int frameNumber = 0; frameNumber < frameCount; frameNumber++)
    {
      if (frameNumber > 0)
      {
        payload += ";"; // Separator between frames
      }
      payload += String(frameNumber) + "," + String(calibrationValues[frameNumber]);
    }
    return payload;
  }
  bool retrieveCalibrationValues(String calibrationReceived)
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
      // Serial.printf("Frame %d: %lu\n", frameNumber, calibrationValue);
      frameStart = nextFrame + 1;
    }
    // Parse the last frame (or only frame if there's just one)
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
      // Serial.printf("Frame %d: %lu\n", frameNumber, calibrationValue);
    }
    return true;
  }
};

// Control functions for the screens
class Display
{
private:
  int csPin; // Chip Select pin
  uint16_t frameCount = 0;
  uint8_t *gifData;
  size_t gifSize;
  uint8_t *pTurboGIFBuffer, *frameGIFBuffer;

public:
  AnimatedGIF gif;
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

  void addGif(uint8_t *gifFile, size_t gifLength)
  {
    frameCount = 0;
    gifData = gifFile;
    gifSize = gifLength;
  }

  int openGif(void)
  {
    gif.begin(GIF_PALETTE_RGB565_BE);
    pTurboGIFBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (imageWidth * imageHeight), MALLOC_CAP_8BIT);
    if (pTurboGIFBuffer == NULL)
    {
      Serial.println("Could not allocate pTurboBuffer");
      return 0;
    }
    frameGIFBuffer = (uint8_t *)malloc(imageWidth * imageHeight * 2);
    if (frameGIFBuffer == NULL)
    {
      Serial.println("Could not allocate frameBuffer");
      return 0;
    }
    gif.setDrawType(GIF_DRAW_COOKED);
    gif.setTurboBuf(pTurboGIFBuffer);
    gif.setFrameBuf(frameGIFBuffer);
    return gif.open(gifData, gifSize, GIFDraw);
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

  void clearScreen(void)
  {
    activate();
    tft.fillScreen(TFT_BLACK);
    deActivate();
  }
  void showText(const char *text, int16_t line, u_int16_t color)
  {
    activate();
    tft.setCursor(10, line);
    tft.setTextColor(color);
    tft.println(text);
    deActivate();
  }
};