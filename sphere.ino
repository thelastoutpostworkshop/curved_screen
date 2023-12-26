#include <Arduino.h>
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

    initDisplay();
    for (int i = 0; i < NUM_DISPLAYS; i++)
    {
        display[i] = new Display(gifToLoad[i].displayCS, gifToLoad[i].image, gifToLoad[i].imageSize);
        if (!display[i]->imageReady)
        {
            Serial.println("Image not ready, cannot continue");
            return;
        }
    }

    Serial.print("Free PSRAM: ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" bytes");

    // int currentDisplay = 0;
    // while (true)
    // {
    //     display[currentDisplay]->showFrame();
    //     currentDisplay++;
    //     if (currentDisplay == NUM_DISPLAYS)
    //     {
    //         currentDisplay = 0;
    //     }
    // }
}

void loop()
{
}
