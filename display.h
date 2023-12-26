#include "gifdec.h"

class Display
{
private:
    int csPin;            // Chip Select pin
    const uint8_t *image; // GIF Image in C
    size_t imageSize;
    uint16_t currentLoop = 0;

public:
    uint8_t *frame; // Frame ready to show
    GIF *gif;       // GIF Image
    bool imageReady = false;
    // Constructor
    Display(int pin, const uint8_t *image, size_t imageSize)
        : csPin(pin), frame(NULL), gif(NULL), image(image), imageSize(imageSize)
    {
        gif = new GIF();
        imageReady = gif->gd_open_gif_memory(image, imageSize);
        if (imageReady)
        {
            Serial.printf("canvas size: %ux%u\n", gif->info()->width, gif->info()->height);
            Serial.printf("number of colors: %d\n", gif->info()->palette->size);
            Serial.printf("number of frames: %d\n", gif->info()->loop_count);
            size_t bufferLength = gif->info()->width * gif->info()->height * colorOutputSize;
            frame = (uint8_t *)malloc(bufferLength);
            if (frame == NULL)
            {
                Serial.println("Not enough memory for buffer");
                imageReady = false;
            }
            else
            {

                imageReady = true;
            }
        }
    }

    void getFrame(void)
    {
        currentLoop++;
        if (currentLoop == gif->info()->loop_count)
        {
            currentLoop = 0;
            gif->gd_rewind();
        }
        gif->gd_get_frame();
        gif->gd_render_frame(frame);
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
};