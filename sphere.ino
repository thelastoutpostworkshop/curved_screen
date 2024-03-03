
#include "sphere.h"
#include "displayRaw.h"
#include "ESP.h"
#include "webserver.h"

#define FRAME_BUFFER_SIZE 50000L
uint8_t *frameBuffer;
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
    {.csPin = 8, .rotation = 3},
    {.csPin = 3, .rotation = 3}};

#ifdef MASTER
Calibration calibration;
#endif

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

ErrorCode getJPGFrames(void)
{
    String frameText = "";
    framesCount = getFramesCount();
    // Serial.printf("Frames Count = %d\n", framesCount);

    if (framesCount <= 0)
    {
        return noFrames;
    }

    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        Screen currentScreen = grid[i];
        for (int frameIndex = 0; frameIndex < framesCount; frameIndex++)
        {
            size_t jpgsize = getFrameJPGData(esp_id_s, i, frameIndex, frameBuffer, FRAME_BUFFER_SIZE);
            if (jpgsize == 0)
            {
                return cannotGetJPGFrames;
            }
            currentScreen.display->addNewFrame(frameBuffer, jpgsize);
            frameText = String(frameIndex + 1) + "/" + String(framesCount);
            grid[i].display->clearScreen();
            grid[i].display->showText("Frames", 50, TFT_GREEN);
            grid[i].display->showText(frameText.c_str(), 100, TFT_GREEN);
            yield();
        }
    }
    return noError;
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
            return cannotGetGifFiles;
        }
        currentScreen.display->addGif(gifData, gifLength);
        if (!currentScreen.display->openGif())
        {
            return cannotOpenGifFile;
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
    }
#ifndef MASTER
    sendCalibrationValues(calibration.getCalibrationValues());
#endif
    return true;
}

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

void setup()
{
#ifdef MASTER
    pinMode(PIN_SYNC, OUTPUT);
    digitalWrite(PIN_SYNC, LOW);
    slaves.resetSlavesReady();
#else
    pinMode(PIN_SYNC, INPUT_PULLUP);
#endif

    Serial.begin(115200);
    if (initWebServer() != noError)
    {
        Serial.println("MDNS Failed for the Master, cannot continue");
        while (true)
            ;
    }
    initTFT_eSPI();
    createDisplay();

    frameBuffer = (uint8_t *)malloc(FRAME_BUFFER_SIZE);

    if (frameBuffer == NULL)
    {
        displayErrorMessage("No Memory for Frame Buffer", 40);
        while (true)
            ;
    }

    ESPID = ESP.getEfuseMac();
    esp_id_s = String(ESPID);

    // Retrieve all the JPG Frames
    // ErrorCode res = getJPGFrames();
    // if (res != noError)
    // {
    //     switch (res)
    //     {
    //     case noFrames:
    //         Serial.println("Error: No Frames.");
    //         displayErrorMessage("No Frames", 40);
    //         break;

    //     case cannotGetJPGFrames:
    //         Serial.println("Error: Could not retrieved all the jpg images, cannot continue.");
    //         displayErrorMessage("Could not retrieved all the jpg images", 40);
    //         break;
    //     }
    //     while (true)
    //         ;
    // }

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
        }

        while (true)
            ;
    }

    eraseAllScreen();

    if (!runCalibration())
    {
        displayErrorMessage("Calibration Error", 40);
        while (true)
            ;
    }

    // Show mac number for identification by the server
    Serial.printf("id=%s\n", esp_id_s.c_str());

    String psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    displayNormalMessage(psram.c_str(), 40);
    eraseAllScreen();

#ifdef MASTER
    displayNormalMessage("Waiting for slaves...", 40);
    slaves.waitForAllSlaves();
    if (!processCalibrationData())
    {
        displayErrorMessage("Processing slaves calibration Error", 40);
        while (true)
            ;
    }
#else
    // sendReady();
#endif
}

unsigned long t, durationCalibrated;
int frameNumber = 0;
void loop()
{
#ifdef MASTER
    digitalWrite(PIN_SYNC, HIGH);
    delayMicroseconds(100);      // Short duration for the pulse
    digitalWrite(PIN_SYNC, LOW); // Set the signal LOW again

    durationCalibrated = calibration.getFrameCalibration(frameNumber) + 2;
    t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->activate();
        if (grid[i].display->gif.playFrame(false, NULL) == 0)
        {
            grid[i].display->gif.reset();
            frameNumber = 0;
        }
        grid[i].display->deActivate();
        frameNumber++;
    }
    Serial.printf("Took %ld ms\n", millis() - t);
    while (millis() - t < durationCalibrated)
        ;
        // waitForSlaves();
#else
    // if (digitalRead(PIN_SYNC) == HIGH)
    // {
    //     t = millis();
    //     for (int i = 0; i < SCREEN_COUNT; i++)
    //     {
    //         grid[i].display->showJPGFrames();
    //     }
    //     // sendReady();
    //     Serial.printf("Took %ld ms\n", millis() - t);
    // }

    // GIF
    if (digitalRead(PIN_SYNC) == HIGH)
    {
        t = millis();
        for (int i = 0; i < SCREEN_COUNT; i++)
        {
            grid[i].display->activate();
            // tft.startWrite();
            grid[i].display->gif.playFrame(false, NULL);
            // tft.endWrite();
            grid[i].display->deActivate();
        }
        Serial.printf("Took %ld ms\n", millis() - t);
    }

#endif
}
