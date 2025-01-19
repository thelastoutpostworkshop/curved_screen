#ifndef _RGBSTATUS_H_
#define _RGBSTATUS_H_

// Function prototypes
void turnBuiltInLEDBlue(uint8_t brightness = 32);
void turnBuiltInLEDGreen(uint8_t brightness = 32);
void turnBuiltInLEDCyan(uint8_t brightness = 32);
void turnBuiltInLEDYellow(uint8_t brightness = 32);
void turnBuiltInLEDRed(uint8_t brightness = 32);

// Function to flash the built-in RGB LED to indicate an error
void flashBuitinRGBError();

#endif // _RGBSTATUS_H_
