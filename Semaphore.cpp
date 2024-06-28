#include "semaphore.h"
#include <stdexcept>

using namespace std;

Semaphore::Semaphore() : IsInitialized(false)
{}

Semaphore::~Semaphore()
{
    if (IsInitialized)
    {
        Destroy();
    }
}

void Semaphore::Init(int value)
{
    if (sem_init(&Sem, 1, value) != 0)
    {
        throw runtime_error("Semaphore initialization failed");
    }
    IsInitialized = true;
}

void Semaphore::Destroy()
{
    sem_destroy(&Sem);
    IsInitialized = false;
}

void Semaphore::Wait()
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

void Semaphore::Release()
{
    if (sem_post(&Sem) != 0)
    {
        throw runtime_error("Error during semaphore release");
    }
}
