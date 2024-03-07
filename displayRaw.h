#include "JPEGDEC.h"     // Install this library with the Arduino IDE Library Manager
#include <TFT_eSPI.h>    // Install this library with the Arduino IDE Library Manager
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager

#define MAX_FRAMES 256
#define colorOutputSize 2 // 16 bit color as output
#define imageWidth 240
#define imageHeight 240

#include "fonts/Prototype20pt7b.h"

TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;

#define BUFFER_SIZE 256 // Optimum is >= GIF width or integral division of width

#ifdef USE_DMA
uint16_t usTemp[2][BUFFER_SIZE]; // Global to support DMA use
#else
uint16_t usTemp[1][BUFFER_SIZE]; // Global to support DMA use
#endif
bool dmaBuf = 0;

void GIFDraw(GIFDRAW *pDraw)
{
  // tft.startWrite();
  // Serial.printf("x=%d, y=%d, w=%d, h=%d\n", pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  if (pDraw->y == 0)
  { // set the memory window (once per frame) when the first line is rendered
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display. We requested 'COOKED'big-endian RGB565 and
  // the library provides them here. No need to do anything except push them right to the display
  tft.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth);
  // tft.endWrite();
} /* GIFDraw() *

// Draw a line of image directly on the LCD
void GIFDraw2(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Displ;ay bounds chech and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > imageWidth)
    iWidth = imageWidth - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= imageHeight || pDraw->iX >= imageWidth || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      }           // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
        usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++)
          usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
          usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
} /* GIFDraw() */

void initTFT_eSPI(void)
{
  tft.init();
  // tft.setSwapBytes(true);
  tft.setFreeFont(&Prototype20pt7b);
}

int draw(JPEGDRAW *pDraw)
{
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

class Calibration
{
private:
  unsigned long calibrationValues[MAX_FRAMES];
  int frameCount;

  bool setCalibration(int frameNumber, unsigned long value)
  {
    if (frameNumber >= MAX_FRAMES)
    {
      Serial.printf("Error:setCalibration frameNumber %d wrong\n", frameNumber);
      return false;
    }
    if (calibrationValues[frameNumber] < value)
    {
      calibrationValues[frameNumber] = value;
    }
    return true;
  }

public:
  Calibration()
  {
    for (int i = 0; i < MAX_FRAMES; i++)
    {
      calibrationValues[i] = 0;
    }
    frameCount = 0;
  }
  void startCalibration(void)
  {
    frameCount = 0;
  }
  int getFrameCount(void)
  {
    return frameCount;
  }
  unsigned long getFrameCalibration(int frameNumber)
  {
    if (frameNumber >= MAX_FRAMES)
    {
      Serial.printf("Error:getFrameCalibration frameNumber %d wrong\n", frameNumber);
      return ULONG_MAX;
    }
    return calibrationValues[frameNumber];
  }
  bool addCalibration(unsigned long value)
  {
    if (frameCount >= MAX_FRAMES)
    {
      Serial.println("Error: Maximum frame count reached.");
      return false;
    }
    if (calibrationValues[frameCount] < value)
    {
      calibrationValues[frameCount] = value;
    }
    frameCount++;
    return true;
  }

  String getCalibrationValues(void)
  {
    String payload = "";
    for (int frameNumber = 0; frameNumber < frameCount; frameNumber++)
    {
      if (frameNumber > 0)
      {
        payload += ";"; // Separator between frames
      }
      payload += String(frameNumber) + "," + String(calibrationValues[frameNumber]);
    }
    return payload;
  }
  bool retrieveCalibrationValues(String calibrationReceived)
  {
    int frameStart = 0;
    int nextFrame;
    while ((nextFrame = calibrationReceived.indexOf(';', frameStart)) != -1)
    {
      String frameData = calibrationReceived.substring(frameStart, nextFrame);
      int commaIndex = frameData.indexOf(',');
      int frameNumber = frameData.substring(0, commaIndex).toInt();
      unsigned long calibrationValue = frameData.substring(commaIndex + 1).toInt();
      if (!setCalibration(frameNumber, calibrationValue))
      {
        return false;
      }
      // Serial.printf("Frame %d: %lu\n", frameNumber, calibrationValue);
      frameStart = nextFrame + 1;
    }
    // Parse the last frame (or only frame if there's just one)
    String lastFrameData = calibrationReceived.substring(frameStart);
    if (lastFrameData.length() > 0)
    {
      int commaIndex = lastFrameData.indexOf(',');
      int frameNumber = lastFrameData.substring(0, commaIndex).toInt();
      unsigned long calibrationValue = lastFrameData.substring(commaIndex + 1).toInt();
      if (!setCalibration(frameNumber, calibrationValue))
      {
        return false;
      }
      // Serial.printf("Frame %d: %lu\n", frameNumber, calibrationValue);
    }
    return true;
  }
};

class Display
{
private:
  int csPin; // Chip Select pin
  uint16_t currentFrame = 0;
  uint16_t frameCount = 0;
  uint8_t *frames[MAX_FRAMES];
  size_t framesSize[MAX_FRAMES];
  uint8_t *pTurboGIFBuffer, *frameGIFBuffer;

public:
  AnimatedGIF gif;
  int screenRotation;

  void activate(void)
  {
    digitalWrite(csPin, LOW);
    tft.setRotation(screenRotation);
  }
  void deActivate(void)
  {
    digitalWrite(csPin, HIGH);
  }
  int chipSelectPin() const
  {
    return csPin;
  }

  uint8_t *addNewFrame(void)
  {
    if (frameCount >= MAX_FRAMES)
    {
      Serial.println("Error: Maximum frame count reached.");
      return NULL;
    }

    size_t bufferLength = imageWidth * imageHeight * colorOutputSize;
    uint8_t *newFrame = (uint8_t *)malloc(bufferLength);

    if (newFrame == NULL)
    {
      Serial.println("Error: Memory allocation for new frame failed.");
      return NULL;
    }

    frames[frameCount] = newFrame;
    frameCount++;
    return newFrame;
  }

  void addNewFrame(uint8_t *frame, size_t frameSize)
  {
    if (frameCount >= MAX_FRAMES)
    {
      Serial.println("Error: Maximum frame count reached.");
      return;
    }

    // Specify the alignment. For example, align to a 8-byte boundary.
    const size_t alignment = 8;
    void *newFrame = NULL;

    // Allocate memory with posix_memalign
    int result = posix_memalign(&newFrame, alignment, frameSize);
    if (result != 0)
    {
      newFrame = NULL; // posix_memalign doesn't set pointer on failure, unlike malloc
      Serial.println("Error: Memory allocation for new frame failed.");
      return;
    }

    memcpy(newFrame, frame, frameSize);
    framesSize[frameCount] = frameSize;
    frames[frameCount] = (uint8_t *)newFrame; // Cast is safe because posix_memalign guarantees alignment
    frameCount++;
  }

  void addGif(uint8_t *gifFile, size_t gifSize)
  {
    frameCount = 0;
    frames[frameCount] = gifFile;
    framesSize[frameCount] = gifSize;
  }

  int openGif(void)
  {
    gif.begin(GIF_PALETTE_RGB565_BE);
    pTurboGIFBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (imageWidth * imageHeight), MALLOC_CAP_8BIT);
    if (pTurboGIFBuffer == NULL)
    {
      Serial.println("Could not allocate pTurboBuffer");
      return 0;
    }
    frameGIFBuffer = (uint8_t *)malloc(imageWidth * imageHeight * 2);
    if (frameGIFBuffer == NULL)
    {
      Serial.println("Could not allocate frameBuffer");
      return 0;
    }
    gif.setDrawType(GIF_DRAW_COOKED);
    gif.setTurboBuf(pTurboGIFBuffer);
    gif.setFrameBuf(frameGIFBuffer);
    return gif.open(frames[0], framesSize[0], GIFDraw);
  }

  Display(int pin, int rotation) : csPin(pin), screenRotation(rotation)
  {
    pinMode(csPin, OUTPUT);
    activate();
    tft.fillScreen(TFT_BLACK);
    deActivate();
  }
  uint16_t getFrameCount(void)
  {
    return frameCount;
  }
  uint8_t *getFrame(int frame)
  {
    return frames[frame];
  }
  size_t getFrameSize()
  {
    return imageWidth * imageHeight * colorOutputSize;
  }
  void freeFrames(void)
  {
    for (int i = 0; i < frameCount; i++)
    {
      free(frames[i]);
    }
    frameCount = 0;
    currentFrame = 0;
  }
  void showFrame(int frame)
  {
    if (frame < frameCount)
    {
      activate();
      tft.pushImage(0, 0, imageWidth, imageHeight, (uint16_t *)frames[frame]);
      deActivate();
    }
    else
    {
      Serial.printf("!!! No frame %d\n", frame);
    }
  }
  void showFrames(void)
  {
    if (frameCount > 0)
    {
      activate();
      tft.pushImage(0, 0, imageWidth, imageHeight, (uint16_t *)frames[currentFrame]);
      deActivate();
      currentFrame++;
      if (currentFrame == frameCount)
      {
        currentFrame = 0;
      }
    }
    else
    {
      Serial.println("!!! No frames to show");
    }
  }
  void showJPGFrames(void)
  {
    if (frameCount > 0)
    {
      int res = jpeg.openRAM(frames[currentFrame], framesSize[currentFrame], draw);
      activate();
      while (jpeg.decode(0, 0, 0) == JPEG_SUCCESS)
      {
        /* jpg file is decoded */
      }
      deActivate();
      jpeg.close();

      currentFrame++;
      if (currentFrame == frameCount)
      {
        currentFrame = 0;
      }
    }
    else
    {
      Serial.println("!!! No frames to show");
    }
  }
  void clearScreen(void)
  {
    activate();
    tft.fillScreen(TFT_BLACK);
    deActivate();
  }
  void showText(const char *text, int16_t line, u_int16_t color)
  {
    activate();
    tft.setCursor(10, line);
    tft.setTextColor(color);
    tft.println(text);
    deActivate();
  }
};