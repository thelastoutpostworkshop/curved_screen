
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
void waitForSlaves(void)
{
    slavesReady = 0;
    while (slavesReady < SLAVECOUNT)
    {
        delay(100);
    }
}

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
    Serial.printf("Frames Count = %d\n", framesCount);

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
}
void displayNormalMessage(const char *message, int16_t line)
{
    grid[0].display->clearScreen();
    grid[0].display->showText(message, line, TFT_GREEN);
}

void setup()
{
#ifdef MASTER
    pinMode(10, OUTPUT);
    digitalWrite(10, LOW);
#else
    pinMode(10, INPUT_PULLUP);
    sendReady();
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
        Serial.println("Error: Memory allocation failed for frame buffer, cannot continue.");
        displayErrorMessage("No Memory for Frame Buffer", 40);
        while (true)
            ;
    }

    ESPID = ESP.getEfuseMac();
    esp_id_s = String(ESPID);

#ifdef MASTER
    displayNormalMessage("Waiting for slaves...", 40);
    waitForSlaves();
#endif

    // Retrieve all the JPG Frames
    ErrorCode res = getJPGFrames();
    if (res != noError)
    {
        switch (res)
        {
        case noFrames:
            Serial.println("Error: No Frames.");
            displayErrorMessage("No Frames", 40);
            break;

        case cannotGetJPGFrames:
            Serial.println("Error: Could not retrieved all the jpg images, cannot continue.");
            displayErrorMessage("Could not retrieved all the jpg images", 40);
            break;
        }
        while (true)
            ;
    }

    // Show mac number for identification by the server
    Serial.printf("id=%s\n", esp_id_s.c_str());

    Serial.printf("PSRAM left = %s\n", formatBytes(ESP.getFreePsram()).c_str());
    String psram = "PSRAM left=" + formatBytes(ESP.getFreePsram());
    displayNormalMessage(psram.c_str(), 40);
}

unsigned long t;
void loop()
{
#ifdef MASTER
    digitalWrite(10, HIGH);
    delayMicroseconds(100); // Short duration for the pulse
    digitalWrite(10, LOW);  // Set the signal LOW again

    // broadcastCommand("Start");
    t = millis();
    for (int i = 0; i < SCREEN_COUNT; i++)
    {
        grid[i].display->showJPGFrames();
    }
    Serial.printf("Took %ld ms\n", millis() - t);
    while (millis() - t < 160)
        ;
        // waitForSlaves();
#else
    if (digitalRead(10) == HIGH)
    {
        t = millis();
        for (int i = 0; i < SCREEN_COUNT; i++)
        {
            grid[i].display->showJPGFrames();
        }
        // sendReady();
        Serial.printf("Took %ld ms\n", millis() - t);
        Serial.printf("Took %ld ms, pin sync=%d\n", millis() - t, digitalRead(10));
    }
#endif
}
