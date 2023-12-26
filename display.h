#include <TFT_eSPI.h> // Install this library with the Arduino IDE Library Manager
#include "gifdec.h"

TFT_eSPI tft = TFT_eSPI(); // Make sure SPI_FREQUENCY is 20000000 in your TFT_eSPI driver for your display if on a breadboard

void initDisplay(void)
{
    tft.init();
    tft.setFreeFont(&FreeSans9pt7b);
}

class Display
{
private:
    int csPin;            // Chip Select pin
    const uint8_t *image; // GIF Image in C
    size_t imageSize;
    uint16_t currentFrame = 0;
    uint16_t frameCount = 0;
    uint8_t *frames[256];
    bool getFrames(void)
    {
        size_t bufferLength = gif->info()->width * gif->info()->height * colorOutputSize;
        while (gif->gd_get_frame())
        {
            frames[frameCount] = (uint8_t *)malloc(bufferLength);
            if (frames[frameCount] != NULL)
            {
                gif->gd_render_frame(frames[frameCount] );
                frameCount++;
            } else {
                return false;
            }
        }
        return true;
    }
    void getFrame(void)
    {
        if (gif->gd_get_frame())
        {
            gif->gd_render_frame(frame);
        }
        else
        {
            gif->gd_rewind();
            getFrame();
        }
    }

    void activate(void)
    {
        digitalWrite(csPin, LOW);
    }
    void deActivate(void)
    {
        digitalWrite(csPin, HIGH);
    }

    int chipSelectPin() const
    {
        return csPin;
    }

public:
    uint8_t *frame; // Frame ready to show
    GIF *gif;       // GIF Image
    bool imageReady = false;
    // Constructor
    Display(int pin, const uint8_t *image, size_t imageSize)
        : csPin(pin), frame(NULL), gif(NULL), image(image), imageSize(imageSize)
    {
        pinMode(csPin, OUTPUT);
        activate();
        tft.setRotation(0); // Adjust orientation as needed (0-3)
        tft.fillScreen(TFT_BLACK);
        deActivate();
        gif = new GIF();
        imageReady = gif->gd_open_gif_memory(image, imageSize);
        if (imageReady)
        {
            // Serial.printf("canvas size: %ux%u\n", gif->info()->width, gif->info()->height);
            // Serial.printf("number of colors: %d\n", gif->info()->palette->size);
            // Serial.printf("number of frames: %d\n", gif->info()->loop_count);
            // Serial.printf("image size: %u\n", imageSize);
            // size_t bufferLength = gif->info()->width * gif->info()->height * colorOutputSize;
            // frame = (uint8_t *)malloc(bufferLength);
            // if (frame == NULL)
            // {
            //     Serial.println("Not enough memory for buffer");
            //     imageReady = false;
            // }
            // else
            // {

            //     imageReady = true;
            // }
            if(!getFrames()) {
                Serial.printf("!!! Not enough memory for frames,stored %ld frames\n",frameCount);
            }
            else {
                Serial.printf("Stored %ld frames\n",frameCount);
            }
        }
    }
    void showFrame(void)
    {
        getFrame();
        activate();
        tft.pushImage(0, 0, gif->info()->width, gif->info()->height, (uint16_t *)frame);
        deActivate();
    }
};