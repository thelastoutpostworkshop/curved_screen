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

    Display *display = new Display(15, x_winglarge, sizeof(x_winglarge));
    Serial.printf("Width=%u, Height=%u\n", display->gif->info()->width, display->gif->info()->height);
    uint8_t *buffer;
    size_t bufferLength = display->gif->info()->width * display->gif->info()->height * colorOutputSize;
    buffer = (uint8_t *)malloc(bufferLength);
    if (buffer == NULL)
    {
        Serial.println("Not enough memory");
        return;
    }
    display->gif->gd_get_frame();
    display->gif->gd_render_frame(buffer);
    uint8_t *buffer1;
    uint8_t *buffer2;
    splitImage(buffer,display->gif->info()->width,display->gif->info()->height,colorOutputSize,buffer1,buffer2);


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

void splitImage(const uint8_t *originalBuffer, int width, int height, int colorSize, uint8_t *buffer1, uint8_t *buffer2)
{
    int newWidth = width / 2;
    int newHeight = height;
    size_t newSize = newWidth * newHeight * colorSize;

    buffer1 = (uint8_t *)malloc(newSize);
    buffer2 = (uint8_t *)malloc(newSize);

    if (buffer1 == NULL || buffer2 == NULL)
    {
        Serial.println("Not enough memory for splitImage");
        free(buffer1);
        free(buffer2);
        return;
    }

    for (int row = 0; row < newHeight; ++row)
    {
        // Copy first half of the row to buffer1
        memcpy(buffer1 + row * newWidth * colorSize,
               originalBuffer + row * width * colorSize,
               newWidth * colorSize);

        // Copy second half of the row to buffer2
        memcpy(buffer2 + row * newWidth * colorSize,
               originalBuffer + (row * width + newWidth) * colorSize,
               newWidth * colorSize);
    }
}

void loop()
{
}
