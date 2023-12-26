#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager
#include "gifdec.h"
#include "display.h"
#include "images/x_wing.h"
#include "images/colortest.h"
#include "images/hyperspace.h"
#include "images/darthvader.h"
#include "images/hud_1.h"

TFT_eSPI tft = TFT_eSPI();  // Make sure SPI_FREQUENCY is 20000000 in your TFT_eSPI driver for your display if on a breadboard
const int NUM_DISPLAYS = 2; // Adjust this value based on the number of displays
Display display[NUM_DISPLAYS] = {
    Display(15), // Assign a chip select pin for each display
    Display(7)};

typedef struct GIFStorage
{
    GIF gif;
    uint8_t *buffer;
    uint64_t *image;
};

const int NUM_GIF = 2;
GIFStorage gifs[NUM_GIF];

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
    if (!initDisplayPinsAndStorage())
    {
        Serial.println("!!! Cannot allocate enough PSRAM to store images");
        Serial.println("!!! Code Execution stopped!");
        return;
    }

    for (int i = 0; i < NUM_GIF; i++)
    {
        /* code */
    }
    

    GIF gif1, gif2;
    uint8_t *buffer1, *buffer2;
    if (gif1.gd_open_gif_memory(hud_1, sizeof(hud_1)))
    {
        Serial.printf("canvas size: %ux%u\n", gif1.info()->width, gif1.info()->height);
        Serial.printf("number of colors: %d\n", gif1.info()->palette->size);
        Serial.printf("Depth: %d\n", gif1.info()->depth);
        size_t bufferLength = gif1.info()->width * gif1.info()->height * colorOutputSize;
        buffer1 = (uint8_t *)malloc(bufferLength);
        if (buffer1 == NULL)
        {
            Serial.println("Not enough memory for buffer");
            return;
        }
    }
    if (gif2.gd_open_gif_memory(x_wing, sizeof(x_wing)))
    {
        Serial.printf("canvas size: %ux%u\n", gif1.info()->width, gif1.info()->height);
        Serial.printf("number of colors: %d\n", gif1.info()->palette->size);
        Serial.printf("Depth: %d\n", gif1.info()->depth);
        size_t bufferLength = gif1.info()->width * gif1.info()->height * colorOutputSize;
        buffer1 = (uint8_t *)malloc(bufferLength);
        if (buffer1 == NULL)
        {
            Serial.println("Not enough memory for buffer");
            return;
        }
    }

    if (gif1.gd_open_gif_memory(hud_1, sizeof(hud_1)))
    {
        Serial.printf("canvas size: %ux%u\n", gif1.info()->width, gif1.info()->height);
        Serial.printf("number of colors: %d\n", gif1.info()->palette->size);
        Serial.printf("Depth: %d\n", gif1.info()->depth);
        size_t bufferLength = gif1.info()->width * gif1.info()->height * colorOutputSize;
        uint8_t *buffer = (uint8_t *)malloc(bufferLength);
        if (buffer != NULL)
        {
            for (unsigned looped = 1;; looped++)
            {
                while (gif1.gd_get_frame())
                {
                    gif1.gd_render_frame(buffer);
                    display[0].activate();
                    tft.pushImage(0, 0, gif1.info()->width, gif1.info()->height, (uint16_t *)buffer);
                    display[0].deActivate();
                }
                if (looped == gif1.info()->loop_count)
                    break;
                gif1.gd_rewind();
            }
            free(buffer);
        }
        else
        {
            Serial.println("Not enough memory for buffer");
        }
        Serial.println("Closing GIF");
        gif1.gd_close_gif();
    }
    else
    {
        Serial.println("Cannot open GIF");
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
        pinMode(display[i].chipSelectPin(), OUTPUT);
        display[i].activate();
        tft.setRotation(1); // Adjust orientation as needed (0-3)
        tft.fillScreen(TFT_BLACK);
        display[i].deActivate();
        display[i].fileIndex = i;
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
