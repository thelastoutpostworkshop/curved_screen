#include <Arduino.h>
#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager
#include "display.h"
#include "images/x_wing.h"
#include "images/colortest.h"
#include "images/hyperspace.h"
#include "images/darthvader.h"
#include "images/hud_1.h"

typedef struct
{
    int displayCS;
    const uint8_t *image;
    size_t imageSize;
} gif_load;

TFT_eSPI tft = TFT_eSPI();  // Make sure SPI_FREQUENCY is 20000000 in your TFT_eSPI driver for your display if on a breadboard
const int NUM_DISPLAYS = 2; // Adjust this value based on the number of displays
gif_load gifToLoad[NUM_DISPLAYS] = {{15, x_wing, sizeof(x_wing)}, {7, hud_1, sizeof(hud_1)}};
Display *display[NUM_DISPLAYS];

void setup()
{
    Serial.begin(115200);
    if (!psramFound())
    {
        Serial.println("!!! No PSRAM detected, cannot continue");
        return;
    }
    else
    {
        Serial.printf("PSRAM Size=%ld\n", ESP.getPsramSize());
    }

    for (int i = 0; i < NUM_DISPLAYS; i++)
    {
        display[i] = new Display(gifToLoad[i].displayCS, gifToLoad[i].image, gifToLoad[i].imageSize);
        if (!display[i]->imageReady)
        {
            Serial.println("Image not ready, cannot continue");
            return;
        }
    }
    if (!initDisplayPinsAndStorage())
    {
        Serial.println("!!! Cannot allocate enough PSRAM to store images");
        Serial.println("!!! Code Execution stopped!");
        return;
    }
    int currentDisplay = 0;
    while (true)
    {
        display[currentDisplay]->getFrame();
        display[currentDisplay]->activate();
        tft.pushImage(0, 0, display[currentDisplay]->gif->info()->width, display[currentDisplay]->gif->info()->height, (uint16_t *)display[currentDisplay]->frame);
        display[currentDisplay]->deActivate();
        currentDisplay++;
        if (currentDisplay == NUM_DISPLAYS)
        {
            currentDisplay = 0;
        }
    }
}

void loop()
{
}

bool initDisplayPinsAndStorage(void)
{
    tft.init();
    tft.setFreeFont(&FreeSans9pt7b);

    for (int i = 0; i < NUM_DISPLAYS; i++)
    {
        pinMode(display[i]->chipSelectPin(), OUTPUT);
        display[i]->activate();
        tft.setRotation(0); // Adjust orientation as needed (0-3)
        tft.fillScreen(TFT_BLACK);
        display[i]->deActivate();
    }
    // for (int i = 0; i < NUM_DISPLAYS; i++)
    // {
    //     if (!display[i].reserveMemoryForStorage())
    //     {
    //         return false;
    //     }
    // }
    return true;
}
