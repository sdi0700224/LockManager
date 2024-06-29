#pragma once
#include "RecordEditor.h"


class Server
{
private:
    const int ReaderWaitingMaxInSec = 10;
    const int ReaderMaxRange = 150;
    const int WriterWaitingMaxInSec = 3;
    const int WriterMaxValue = 1000;

    unsigned int Seed;

    char* Filename;
    int MaxNumOfSegments;
    RecordEditor Editor;
    int SegmentsNum;
    int SegmentShmFD;
    char* ShmSegmentName;
    size_t TotalSizeCS;
    void* MappedMemory;
    
    int GetBlockNumber(int custId);
    void InitializeCS();
    void DestroyCS();

public:
    struct Statistics
    {
        int NumOfReaders;
        int NumOfWriters;
        long NumOfRecordsProcessed;
        double TotalReaderTime;
        double TotalWritterTime;
        double MaxDelay;
    };
    
	Server(const char* filename, int MaxNumOfSegments);
	~Server();

	void Init();
    void CreateAndWaitClients(int numOfReaders, int numOfWriters);
    void PrintStatistics();
};