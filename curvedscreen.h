#ifndef _CURVEDSCREEN_
#define _CURVEDSCREEN_
#include "slaves.h"

#define PIN_SYNC_SHOW_FRAME 38

int SAFETY_WAIT_TIME_FRAME = 5;

enum ErrorCode
{
    noError,
    noFrames,
    cannotGetJPGFrames,
    cannotGetGifFiles,
    cannotOpenGifFile,
    notEnoughMemory,
    cannotSendCalibrationValues,
    noMDNS
};

ErrorCode lastError;

#endif