#ifndef _SLAVES_H_
#define _SLAVES_H_

#include <Arduino.h>
#include "configure.h"

class SLAVES
{
private:
    int calibrationReceived;
    String CalibrationData[SLAVECOUNT];

    // Check if all slaves are ready
    bool allSlavesReady(void);

public:
    // Constructor
    SLAVES();

    // Reset the ready state of slaves
    void resetSlavesReady(void);

    // Add calibration data for a slave
    void addCalibrationData(String data);

    // Get the calibration data for a specific slave
    String getCalibrationData(int slaveNumber);

    // Block until all slaves are ready
    void waitForAllSlaves(void);
};

#endif // _SLAVES_H_
