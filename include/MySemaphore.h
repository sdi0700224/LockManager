#pragma once
#include <semaphore.h>

class MySemaphore
{
public:
    MySemaphore();
    ~MySemaphore();

    void Init(int value);
    void Destroy();
    void Wait();
    void Release();

private:
    sem_t Sem;
    bool IsInitialized;
};
