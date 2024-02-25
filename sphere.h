#ifndef _SPHERE_
#define _SPHERE_

#define MASTER // When uncommented this means this ESP32 is the master

#ifdef MASTER
#define SLAVECOUNT 1 // The number of ESP32 slaves
#define SERVERNAME "sphere"
#endif

enum ErrorCode
{
    noError,
    noFrames,
    cannotGetJPGFrames,
    noMDNS
};

#endif