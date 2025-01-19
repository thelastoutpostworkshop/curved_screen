#include "slaves.h"
#include "configure.h"

// Constructor
SLAVES::SLAVES()
    : calibrationReceived(0) // Initialize calibrationReceived to 0
{
}

// Check if all slaves are ready
bool SLAVES::allSlavesReady(void)
{
    return calibrationReceived == SLAVECOUNT;
}

// Reset the ready state of slaves
void SLAVES::resetSlavesReady(void)
{
    calibrationReceived = 0;
}

// Add calibration data for a slave
void SLAVES::addCalibrationData(String data)
{
    if (calibrationReceived < SLAVECOUNT)
    {
        CalibrationData[calibrationReceived] = data;
        calibrationReceived++;
    }
    else
    {
        Serial.println("Too many calibration data received!");
    }
}

// Get the calibration data for a specific slave
String SLAVES::getCalibrationData(int slaveNumber)
{
    if (slaveNumber >= SLAVECOUNT)
    {
        Serial.printf("Slave number %d non-existent\n", slaveNumber);
        return "";
    }
    return CalibrationData[slaveNumber];
}

// Block until all slaves are ready
void SLAVES::waitForAllSlaves(void)
{
    while (!allSlavesReady())
    {
        yield();
    }
}
