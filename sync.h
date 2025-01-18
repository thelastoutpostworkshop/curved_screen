#ifndef _SYNC_
#define _SYNC_
#include "slaves.h"

#define PIN_SYNC_SHOW_FRAME 38 // Pin to used for sync signal to trigger slaves for showing a frame

#ifdef MASTER
SLAVES slaves;  // Slaves
#endif

// Error codes
enum ErrorCode
{
    noError,
    noFrames,
    cannotGetGifFiles,
    cannotOpenGifFile,
    notEnoughMemory,
    cannotSendCalibrationValues,
    noMDNS
};

ErrorCode lastError;    // Contain the last error code

#endif