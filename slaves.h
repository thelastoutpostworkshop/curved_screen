#ifndef _SLAVES_
#define _SLAVES_

#define SLAVECOUNT 1 // The number of ESP32 slaves

class SLAVES
{
private:
    int slavesReady;
    String CalibrationData[SLAVECOUNT];

    bool allSlavesReady(void)
    {
        if (slavesReady == SLAVECOUNT)
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
        slavesReady = 0;
    }
    void addSlavesReady(void)
    {
        slavesReady++;
    }
    void addCalibrationData(String data)
    {
        CalibrationData[slavesReady] = data;
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