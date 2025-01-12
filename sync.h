#ifndef _SPHERE_
#define _SPHERE_
#include "slaves.h"

#define SERVERNAME "sphere"
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