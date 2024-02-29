#ifndef _SPHERE_
#define _SPHERE_
#include "slaves.h"

SLAVES *slaves;

// #define MASTER // When uncommented this means this ESP32 is the master

#define SERVERNAME "sphere"
#define PIN_SYNC 10

#ifdef MASTER
#define SLAVECOUNT 1 // The number of ESP32 slaves
#endif

enum ErrorCode
{
    noError,
    noFrames,
    cannotGetJPGFrames,
    noMDNS
};

#endif