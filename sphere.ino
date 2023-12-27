#include <Arduino.h>
#include "display.h"
#include "images/x_wing.h"
#include "images/x_winglarge.h"
#include "images/colortest.h"
#include "images/hyperspace.h"
#include "images/darthvader.h"
#include "images/hud_1.h"
#include "images/bb8.h"

typedef struct
{
    int displayCS;
    const uint8_t *image;
    size_t imageSize;
} gif_load;

const int NUM_DISPLAYS = 2; // Adjust this value based on the number of displays
gif_load gifToLoad[NUM_DISPLAYS] = {{15, bb8, sizeof(bb8)}, {7, x_wing, sizeof(x_wing)}};
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

    Display display = new Display(15,x_wing, gifToLoad[i].imageSize);

    // for (int i = 0; i < NUM_DISPLAYS; i++)
    // {
    //     display[i] = new Display(gifToLoad[i].displayCS, gifToLoad[i].image, gifToLoad[i].imageSize);
    //     if (!display[i]->imageReady)
    //     {
    //         Serial.println("Image not ready, cannot continue");
    //         return;
    //     }
    // }

    // Serial.print("Free PSRAM: ");
    // Serial.print(ESP.getFreePsram());
    // Serial.println(" bytes");

    // int currentDisplay = 0;
    // while (true)
    // {
    //     unsigned long speed = millis();
    //     display[currentDisplay]->showFrames();
    //     Serial.printf("Screen 0 FPS=%f\n", 1000.0f / (millis() - speed));

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
