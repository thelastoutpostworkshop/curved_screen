// Slaves functions
//
#ifndef _SLAVES_
#define _SLAVES_

class SLAVES
{
private:
    int calibrationReceived;
    String CalibrationData[SLAVECOUNT];

    // Wait for all slaves calibration data to be received
    bool allSlavesReady(void)
    {
        if (calibrationReceived == SLAVECOUNT)
        {
            return true;
        }
        return false;
    }

public:
    SLAVES()
    {
    }
    void resetSlavesReady(void)
    {
        calibrationReceived = 0;
    }
    void addCalibrationData(String data)
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

    String getCalibrationData(int slaveNumber)
    {
        if (slaveNumber >= SLAVECOUNT)
        {
            Serial.printf("Slave number %d non existant\n", slaveNumber);
            return "";
        }
        return CalibrationData[slaveNumber];
    }

    // Wait for all slaves to be ready
    void waitForAllSlaves(void)
    {
        while (!allSlavesReady())
        {
            yield();
        }
    }
};
#endif