#ifndef _SPHERE_
#define _SPHERE_
#include "slaves.h"

// #define MASTER // When uncommented this means this ESP32 is the master

#define SERVERNAME "sphere"
#define PIN_SYNC 10

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
    noMDNS
};

#endif