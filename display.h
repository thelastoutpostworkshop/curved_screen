#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>    // Install this library with the Arduino IDE Library Manager
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager
#include "configure.h"

// Functions
void GIFDraw(GIFDRAW *pDraw);
void initTFT_eSPI(void);

// Calibration class
class Calibration
{
private:
    unsigned long calibrationValues[MAX_FRAMES];
    int frameCount;
    bool setCalibration(int frameNumber, unsigned long value);

public:
    Calibration();
    void startCalibration(void);
    int getFrameCount(void);
    unsigned long getFrameCalibration(int frameNumber);
    bool addCalibration(unsigned long value);
    String getCalibrationValues(void);
    bool retrieveCalibrationValues(String calibrationReceived);
};

// Display class
class Display
{
private:
    int csPin;
    uint16_t frameCount;
    uint8_t *gifData;
    size_t gifSize;
    uint8_t *pTurboGIFBuffer, *frameGIFBuffer;

public:
    AnimatedGIF gif;
    int screenRotation;

    Display(int pin, int rotation);
    ~Display();
    void activate(void);
    void deActivate(void);
    int chipSelectPin() const;
    void addGif(uint8_t *gifFile, size_t gifLength);
    int openGif(void);
    uint16_t getFrameCount(void);
    void clearScreen(void);
    void showText(const char *text, int16_t line, uint16_t color);
};

#endif // DISPLAY_H
