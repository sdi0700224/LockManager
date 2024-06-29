#include "MySemaphore.h"
#include <stdexcept>

using namespace std;

MySemaphore::MySemaphore() : IsInitialized(false)
{}

MySemaphore::~MySemaphore()
{
    if (IsInitialized)
    {
        Destroy();
    }
}

void MySemaphore::Init(int value)
{
    if (sem_init(&Sem, 1, value) != 0)
    {
        throw runtime_error("Semaphore initialization failed");
    }
    IsInitialized = true;
}

void MySemaphore::Destroy()
{
    sem_destroy(&Sem);
    IsInitialized = false;
}

void MySemaphore::Wait()
{
    int retValue;
    do
    {
        retValue = sem_wait(&Sem);
    }
    while (retValue == -1 && errno == EINTR);

    if (retValue != 0)
    {
        throw runtime_error("Error during semaphore wait");
    }
}

void MySemaphore::Release()
{
    if (sem_post(&Sem) != 0)
    {
        throw runtime_error("Error during semaphore release");
    }
}
