
#include "sphere.h"
#include "displayRaw.h"
#include "ESP.h"
#include "webserver.h"

int framesCount;
String esp_id_s;
uint64_t ESPID;

typedef struct
{
    int csPin;
    int rotation;
    Display *display;
} Screen;

#define SCREEN_COUNT 4

Screen grid[SCREEN_COUNT] = {
    {.csPin = 17, .rotation = 3},
    {.csPin = 18, .rotation = 3},
    {.csPin = 47, .rotation = 3},
    {.csPin = 48, .rotation = 3}};

Calibration calibration;

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

ErrorCode getGifFiles(void)
{
    uint8_t *gifData;
    size_t gifLength;
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Screen currentScreen = grid[i];

        currentScreen.display->clearScreen();
        currentScreen.display->showText("Getting GIF File", 50, TFT_GREEN);

        gifData = getGifData(esp_id_s, i, &gifLength);
        if (gifData == NULL)
        {
            return lastError;
        }
        currentScreen.display->addGif(gifData, gifLength);
        if (!currentScreen.display->openGif())
        {
            return lastError;
        }
        currentScreen.display->clearScreen();
        currentScreen.display->showText("Completed", 50, TFT_GREEN);
        yield();
    }
    return noError;
}

String formatBytes(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(bytes) + " B";
    }
    else if (bytes < (1024 * 1024))
    {
        return String(bytes / 1024.0, 2) + " KB";
    }
    else
    {
        return String(bytes / 1024.0 / 1024.0, 2) + " MB";
    }
}

void displayErrorMessage(char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_ORANGE);
    Serial.println(message);
}
void displayNormalMessage(const char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_GREEN);
    Serial.println(message);
}

void eraseAllScreen(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->clearScreen();
    }
}

bool runCalibration(void)
{
    int calibrationLoop = 2;
    unsigned long duration, t;
    bool moreFrame = true;

    for (int cal = 0; cal < calibrationLoop; cal++)
    {
        for (int i = 0; i < SCREEN_COUNT; i++)
        {
            grid[i].display->gif.reset();
        }
        moreFrame = true;
        calibration.startCalibration();
        displayNormalMessage("Calibrating...", 40);
        while (moreFrame)
        {
            t = millis();
            for (int i = 0; i < SCREEN_COUNT; i++)
            {
                grid[i].display->activate();
                if (grid[i].display->gif.playFrame(false, NULL) == 0)
                {
                    moreFrame = false;
                }
                grid[i].display->deActivate();
            }
            duration = millis() - t;
            if (!calibration.addCalibration(duration))
            {
                return false;
            }
        }
        Serial.println(calibration.getCalibrationValues().c_str());
    }
#ifndef MASTER
    if (sendCalibrationValues(calibration.getCalibrationValues()) != noError)
    {
        displayNormalMessage("Error sending calibration values", 40);
        while (true)
            ;
    }
#endif
    framesCount = calibration.getFrameCount();
    return true;
}

#ifdef MASTER
bool processCalibrationData(void)
{
    for (int i = 0; i < SLAVECOUNT; i++)
    {
        if (!calibration.retrieveCalibrationValues(slaves.getCalibrationData(i)))
        {
            return false;
        }
    }
    return true;
}
#else
void showFrameInterrupt()
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->activate();
        grid[i].display->gif.playFrame(false, NULL);
        grid[i].display->deActivate();
    }
}
#endif

void turnBuiltInLEDBlue(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, 0, brightness);
}
void turnBuiltInLEDGreen(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, brightness, 0);
}
void turnBuiltInLEDCyan(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, brightness, brightness);
}
void turnBuiltInLEDYellow(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, brightness, brightness, 0);
    Serial.printf("Turn RGB yellow, brightness=%d\n", brightness);
}
void turnBuiltInLEDRed(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, brightness, 0, 0);
    Serial.printf("Turn RGB red, brightness=%d\n", brightness);
}

void setup()
{
    String psram;
#ifdef MASTER
    pinMode(PIN_SYNC_SHOW_FRAME, OUTPUT);
    digitalWrite(PIN_SYNC_SHOW_FRAME, LOW);
    slaves.resetSlavesReady();
#else
    pinMode(PIN_SYNC_SHOW_FRAME, INPUT_PULLUP);
    delay(5000); // Give the master the time to start
#endif

    turnBuiltInLEDCyan();

    Serial.begin(115200);
    if (initWebServer() != noError)
    {
        Serial.println("MDNS Failed for the Master, cannot continue");
        turnBuiltInLEDRed();
        while (true)
            ;
    }
    psram = "PSRAM Size=" + formatBytes(ESP.getPsramSize());
    Serial.println(psram.c_str());

    initTFT_eSPI();
    createDisplay();

    ESPID = ESP.getEfuseMac();
    esp_id_s = String(ESPID);
    Serial.printf("ESP id=%s\n", esp_id_s.c_str());

    ErrorCode res = getGifFiles();
    if (res != noError)
    {
        switch (res)
        {
        case cannotGetGifFiles:
            displayErrorMessage("Could not retrieved all the GIF files", 40);
            break;

        case cannotOpenGifFile:
            displayErrorMessage("Could not open a GIF file", 40);
            break;

        case notEnoughMemory:
            displayErrorMessage("Not Enough memory", 40);
            break;
        }
        turnBuiltInLEDRed();
        while (true)
            ;
    }

    eraseAllScreen();

#ifndef MASTER
    attachInterrupt(digitalPinToInterrupt(PIN_SYNC_SHOW_FRAME), showFrameInterrupt, RISING);
#endif

    if (!runCalibration())
    {
        displayErrorMessage("Calibration Error", 40);
        turnBuiltInLEDRed();
        while (true)
            ;
    }

    // Show mac number for identification by the server
    Serial.printf("id=%s\n", esp_id_s.c_str());

    psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    Serial.println(psram.c_str());
    // displayNormalMessage(psram.c_str(), 40);
    // eraseAllScreen();

#ifdef MASTER
    turnBuiltInLEDBlue(128);
    eraseAllScreen();
    turnBuiltInLEDYellow();
    displayNormalMessage("Waiting for slaves...", 40);
    slaves.waitForAllSlaves();
    if (!processCalibrationData())
    {
        displayErrorMessage("Processing slaves calibration Error", 40);
        turnBuiltInLEDRed();
        while (true)
            ;
    }
    Serial.println("Final calibration values:");
    Serial.println(calibration.getCalibrationValues().c_str());
    turnBuiltInLEDBlue();
#else
    turnBuiltInLEDGreen(128);
#endif
}

unsigned long t, durationCalibrated;
int frameNumber = 0;
void loop()
{
#ifdef MASTER
    durationCalibrated = calibration.getFrameCalibration(frameNumber) + SAFETY_WAIT_TIME_FRAME;
    // durationCalibrated = 160;
    // Serial.printf("Calibration frame #%d is %lu ms\n", frameNumber, durationCalibrated);

    digitalWrite(PIN_SYNC_SHOW_FRAME, HIGH);
    delayMicroseconds(100);                 // Short duration for the pulse
    digitalWrite(PIN_SYNC_SHOW_FRAME, LOW); // Set the signal LOW again

    t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->activate();
        grid[i].display->gif.playFrame(false, NULL);
        grid[i].display->deActivate();
    }
    // Serial.printf("Took %lu ms\n", millis() - t);
    frameNumber++;
    if (frameNumber == framesCount)
    {
        frameNumber = 0;
    }
    while ((millis() - t) <= durationCalibrated)
        ;
#endif
}
