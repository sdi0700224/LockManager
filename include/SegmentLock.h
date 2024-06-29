#pragma once
#include "MySemaphore.h"

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
    MySemaphore In;
    MySemaphore Out;
    MySemaphore Wrt;
    int CtrIn;
    int CtrOut;
    bool Wait;
};
