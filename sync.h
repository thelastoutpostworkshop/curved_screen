#ifndef _SYNC_
#define _SYNC_
#include "slaves.h"

#define PIN_SYNC_SHOW_FRAME 38

#ifdef MASTER
SLAVES slaves;
#endif

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