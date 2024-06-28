#include "SegmentLock.h"

SegmentLock::SegmentLock() : CtrIn(0), CtrOut(0), Wait(false)
{}

void SegmentLock::Init()
{
    In.Init(1);
    Out.Init(1);
    Wrt.Init(0);
    CtrIn = 0;
    CtrOut = 0;
    Wait = false;
}

void SegmentLock::Destroy()
{
    In.Destroy();
    Out.Destroy();
    Wrt.Destroy();
}

void SegmentLock::ReaderLock()
{
    In.Wait();
    CtrIn++;
    In.Release();
}

void SegmentLock::ReaderUnlock()
{
    Out.Wait();
    CtrOut++;
    if (Wait && CtrIn == CtrOut)
    {
        Wrt.Release();
    }
    Out.Release();
}

void SegmentLock::WriterLock()
{
    In.Wait();
    Out.Wait();
    if (CtrIn == CtrOut)
    {
        Out.Release();
    }
    else
    {
        Wait = true;
        Out.Release();
        Wrt.Wait();
        Wait = false;
    }
}

void SegmentLock::WriterUnlock()
{
    In.Release();
}
