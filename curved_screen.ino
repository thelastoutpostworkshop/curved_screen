// Main sketch for the master ESP32-S3 and the slaves
// Tested on xxx

// Comment the next line to compile and upload the code to the ESP32-S3 acting as the slaves, you can have as many slaves as you want
// Uncomment the next line to compile and upload the code to the ESP32-S3 acting as the master - only one master is allowed
// #define MASTER  

#define SLAVECOUNT 1 // The number of ESP32-S3 slaves

#include "sync.h"
#include "displayRaw.h"
#include "webserver.h"
#include "rgbstatus.h"

int framesCount;    // The number of frames for the GIF files
String esp_id_s;    // The ESP32-S3 internal mac address, serving as the ID for the GIF processing program

typedef struct
{
    int csPin;
    int rotation;
    Display *display;
} Screen;

// Screen arrangement on the ESP32-S3
// Do not forget to configure the proper setup driver for the TFT_eSPI library, see this tutorial : https://youtu.be/6lLlKY5U77w
#define SCREEN_COUNT 4 // The number of screens connected to the ESP32-S3
Screen grid[SCREEN_COUNT] = { // Set the proper CS pin and orientation for each screen
    {.csPin = 17, .rotation = 0},
    {.csPin = 18, .rotation = 0},
    {.csPin = 48, .rotation = 0},
    {.csPin = 47, .rotation = 0}};

Calibration calibration; // To store calibration data
#define SAFETY_WAIT_TIME_FRAME 5    // Value (in ms) added to the final calibration time of a frame for safety

void createDisplay(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display = new Display(grid[i].csPin, grid[i].rotation);
    }
}

// Reads GIF files for each screen
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

// Format size string in human readable format
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

// Display error message on a sreen
void displayErrorMessage(char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_ORANGE);
    Serial.println(message);
}

// Display information message on a sreen
void displayNormalMessage(const char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_GREEN);
    Serial.println(message);
}

// Erase all screen to black
void eraseAllScreen(void)
{
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->clearScreen();
    }
}

// Run a calibration on the gif to find the time needed to display each frame
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
    // A screen slave will send its calibration data to the master
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
// Retrieve calibration data from all the screen slaves
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

// Display a frame on each screen
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

void setup()
{
    String psram;
#ifdef MASTER
    // Master ESP32-S3 in indicated buy its builtin RGB LED turned green
    turnBuiltInLEDGreen();
    pinMode(PIN_SYNC_SHOW_FRAME, OUTPUT); // The sync pin is controlled by the master
    digitalWrite(PIN_SYNC_SHOW_FRAME, LOW);
    slaves.resetSlavesReady();
#else
    // Master ESP32-S3 in indicated buy its builtin RGB LED turned blue
    turnBuiltInLEDBlue();
    pinMode(PIN_SYNC_SHOW_FRAME, INPUT_PULLUP); // The sync pin will be read by the slaves
    delay(5000);                                // Give the master the time to start
#endif

    Serial.begin(115200);
    if (initWebServer() != noError)
    {
        Serial.println("MDNS Failed for the Master, cannot continue");
        flashBuitinRGBError();
    }
    Serial.printf("RGB builtin=%d\n", RGB_BUILTIN);
    const uint32_t psram_size = ESP.getPsramSize();
    if (psram_size == 0)
    {
        Serial.println("No PSRAM avalaible, cannot continue");
        flashBuitinRGBError();
    }
    psram = "PSRAM Size=" + formatBytes(psram_size);
    Serial.println(psram.c_str());

    initTFT_eSPI();
    createDisplay();

    // Get the internal ESP32-S3 Mac address, this will serve as the ID for the GIF server program
    const uint32_t ESPID = ESP.getEfuseMac();
    esp_id_s = String(ESPID);
    Serial.printf("ESP id=%s\n", esp_id_s.c_str());

    // Get the gif files from the GIF server
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
        flashBuitinRGBError();
    }

    eraseAllScreen();

#ifndef MASTER
    // The sync pin will trigger an interrupt on the slaves and show a frame on all screen
    attachInterrupt(digitalPinToInterrupt(PIN_SYNC_SHOW_FRAME), showFrameInterrupt, RISING);
#endif

    // Start the calibration process
    if (!runCalibration())
    {
        displayErrorMessage("Calibration Error", 40);
        flashBuitinRGBError();
    }

    // Print the internal ESP32-S3 Mac address, this will serve as the ID for the GIF server program
    Serial.printf("id=%s\n", esp_id_s.c_str());

    // Shows the psram left on the ESP32-S3, this will give you an idea on the size remaining if you want larger gif to be displayed
    psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    Serial.println(psram.c_str());
    // displayNormalMessage(psram.c_str(), 40);
    // eraseAllScreen();

#ifdef MASTER
    eraseAllScreen();
    displayNormalMessage("Waiting for slaves...", 40);
    // The master waits for all the calibration data to be received from the slaves, then process the calibration data
    slaves.waitForAllSlaves();
    if (!processCalibrationData())
    {
        displayErrorMessage("Processing slaves calibration Error", 40);
        flashBuitinRGBError();
    }
    Serial.println("Final calibration values:");
    Serial.println(calibration.getCalibrationValues().c_str());
#endif
}

unsigned long t, durationCalibrated;
int frameNumber = 0;
void loop()
{
#ifdef MASTER // Loop is not used on the slaves
    // Get the calibrated duration for the frame
    durationCalibrated = calibration.getFrameCalibration(frameNumber) + SAFETY_WAIT_TIME_FRAME;
    // durationCalibrated = 160;
    // Serial.printf("Calibration frame #%d is %lu ms\n", frameNumber, durationCalibrated);

    // Send the sync signal to the slaves to start showing the frames
    digitalWrite(PIN_SYNC_SHOW_FRAME, HIGH);
    delayMicroseconds(100);                 // Short duration for the pulse
    digitalWrite(PIN_SYNC_SHOW_FRAME, LOW); // Set the signal LOW again

    // The master show its own frames
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
        // This will loop the GIF
        frameNumber = 0;
    }
    // Wait for the calibrated duration to pass
    while ((millis() - t) <= durationCalibrated)
        ;
#endif
}
