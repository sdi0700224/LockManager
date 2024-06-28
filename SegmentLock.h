#pragma once
#include "Semaphore.h"

class SegmentLock
{
public:
    SegmentLock();
    
    void Init();
    void Destroy();
    void ReaderLock();
    void ReaderUnlock();
    void WriterLock();
    void WriterUnlock();

private:
    Semaphore In;
    Semaphore Out;
    Semaphore Wrt;
    int CtrIn;
    int CtrOut;
    bool Wait;
};
