// RGB functions
//
#ifndef _RGB_STATUS_
#define _RGB_STATUS_

// These functions use the builtin RGB Led on the ESP32S3 as a visual indicators
void turnBuiltInLEDBlue(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, 0, brightness);
}
void turnBuiltInLEDGreen(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, brightness, 0);
}
void turnBuiltInLEDCyan(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, 0, brightness, brightness);
}
void turnBuiltInLEDYellow(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, brightness, brightness, 0);
}
void turnBuiltInLEDRed(uint8_t brightness = 32)
{
    neopixelWrite(RGB_BUILTIN, brightness, 0, 0);
}

// Built-in LED RBG blinks as a visual indicator that something has gone wrong
void flashBuitinRGBError()
{
    while (true)
    {
        turnBuiltInLEDRed();
        delay(300);
#ifdef MASTER
        turnBuiltInLEDGreen();
#else
        turnBuiltInLEDBlue();
#endif
        delay(300);
    }
}
#endif
