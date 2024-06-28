#pragma once
#include <semaphore.h>

class Semaphore
{
public:
    Semaphore();
    ~Semaphore();

    void Init(int value);
    void Destroy();
    void Wait();
    void Release();

private:
    sem_t Sem;
    bool IsInitialized;
};
