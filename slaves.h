#ifndef _SLAVES_
#define _SLAVES_

class SLAVES
{
private:
    int slavesCount;
    int slavesReady;

public:
    SLAVES(int numberOfSlaves)
    {
        slavesCount = numberOfSlaves;
    }
    void resetSlavesReady(void)
    {
        slavesReady = 0;
    }
    void addSlavesReady(void)
    {
        slavesReady++;
    }
    bool allSlavesReady(void)
    {
        if (slavesReady == slavesCount)
        {
            return true;
        }
        return false;
    }
    void waitForAllSlaves(void) {
        while (!allSlavesReady())
        {
            yield();
        }
        
    }
};
#endif