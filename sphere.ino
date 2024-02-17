
#include "displayRaw.h"
#include "wear_levelling.h"
#include "frame_001.h"
#include "webserver.h"

uint8_t *fileBuffer = NULL;
size_t fileBufferSize = 0;

typedef struct
{
    int row;
    int column;
    int csPin;
    Display *display;
} Screen;

#define ROWS 1    // Number of rows
#define COLUMNS 2 // Number of columns

Screen grid[ROWS][COLUMNS] = {
    {
        {.row = 0, .column = 0, .csPin = 7}, // Column 0
        {.row = 0, .column = 1, .csPin = 6} // Column 1
    }};

// int totalWidth = imageWidth * COLUMNS;
// int totalHeight = imageHeight * ROWS;

ESP32Server esp32Server;

void setup()
{
    Serial.begin(115200);
    esp32Server.initWebServer();
    initDisplay();

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLUMNS; c++)
        {
            grid[r][c].display = new Display(grid[r][c].csPin);
        }
    }

    grid[0][0].display->activate();
    tft.pushImage(0,0,240,240,frame);
    grid[0][0].display->deActivate();


}

void loop()
{

}


