#ifndef _SLAVES_
#define _SLAVES_

#define SLAVECOUNT 1 // The number of ESP32 slaves

class SLAVES
{
private:
    int calibrationReceived;
    String CalibrationData[SLAVECOUNT];

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

    void waitForAllSlaves(void)
    {
        while (!allSlavesReady())
        {
            yield();
        }
    }
};
#endif