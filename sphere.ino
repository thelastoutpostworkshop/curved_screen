
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
    int rotation;
    Display *display;
} Screen;

#define ROWS 2    // Number of rows
#define COLUMNS 2 // Number of columns

Screen grid[ROWS][COLUMNS] = {
    {{.row = 0, .column = 0, .csPin = 6, .rotation = 0},
     {.row = 0, .column = 1, .csPin = 7, .rotation = 0}},
    {{.row = 1, .column = 0, .csPin = 16, .rotation = 2},
     {.row = 1, .column = 1, .csPin = 17, .rotation = 2}}};

void createDisplay(void)
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLUMNS; c++)
        {
            grid[r][c].display = new Display(grid[r][c].csPin, grid[r][c].rotation);
        }
    }
}

void getFrames(void)
{
    int screenCount = ROWS * COLUMNS;
    int framesCount = getFramesCount();
    Serial.printf("Frames Count = %d\n", framesCount);

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLUMNS; ++j)
        {
            Screen currentScreen = grid[i][j];
            for (int frameIndex = 0; frameIndex < framesCount; frameIndex++)
            {
                u_int8_t *frame = currentScreen.display->addNewFrame();
                if (frame != NULL)
                {
                    getFrameData(i+j, frameIndex, frame, currentScreen.display->getFrameSize());
                }
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    initWebServer();
    initTFT_eSPI();
    createDisplay();

    getFrames();
}

void loop()
{
}
