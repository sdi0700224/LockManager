#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include "Server.h"
#include "SegmentLock.h"

using namespace std;

Server::Server(const char* filename, int maxNumOfSegments) : MaxNumOfSegments(maxNumOfSegments), Editor(filename), SegmentsNum(0)
{
    Filename = new char[strlen(filename) + 1];
    strcpy(Filename, filename);

    ShmSegmentName = new char[32];
    strcpy(ShmSegmentName, "/serverSegmentLocks");

    Seed = static_cast<unsigned int>(time(NULL));
}

Server::~Server()
{
    DestroyCS();
    
    delete[] Filename;
    delete[] ShmSegmentName;
}

void Server::Init()
{
    cout << "Servet Init.." << endl;
    
    int accountNum = Editor.NumOfRecords;
    cout << "Accounts number: " << accountNum << endl;
    cout << "Max Semaphore Sets number: " << MaxNumOfSegments << endl;

    if (accountNum <= 0)
    {
        cerr << "Error, no accounts found in file" << endl;
    }
    
    // Get last account's block number
    SegmentsNum = GetBlockNumber(accountNum);
    cout << "Segments number: " << SegmentsNum << endl << endl;
    
    InitializeCS();
}

int Server::GetBlockNumber(int custId)
{
    int numRecords = Editor.NumOfRecords;
    int recordsPerBlock = (numRecords + MaxNumOfSegments - 1) / MaxNumOfSegments; // Ceiling division

    return ((custId - 1) / recordsPerBlock) + 1;
}

void Server::InitializeCS()
{
    TotalSizeCS = sizeof(int) + (SegmentsNum * sizeof(SegmentLock)) + sizeof(Statistics) + sizeof(Semaphore); // Total size of CS

    // Create and open a new shared memory object
    SegmentShmFD = shm_open(ShmSegmentName, O_CREAT | O_RDWR, 0666);
    if (SegmentShmFD == -1)
    {
        cerr << "Failed to open shared memory object: " << ShmSegmentName << endl;
        return;
    }

    // Set the size of the shared memory object
    if (ftruncate(SegmentShmFD, TotalSizeCS) == -1)
    {
        cerr << "Failed to set size of shared memory object" << endl;
        close(SegmentShmFD);
        return;
    }

    // Map the shared memory object
    MappedMemory = mmap(NULL, TotalSizeCS, PROT_READ | PROT_WRITE, MAP_SHARED, SegmentShmFD, 0);
    if (MappedMemory == MAP_FAILED)
    {
        cerr << "Failed to map shared memory object" << endl;
        close(SegmentShmFD);
        return;
    }

    // Store the number of segments at the beginning of the shared memory
    *(reinterpret_cast<int*>(MappedMemory)) = SegmentsNum;

    // Calculate the starting address for SegmentLock instances
    void* locksMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int);

    // Initialize SegmentLocks in shared memory
    for (int i = 0; i < SegmentsNum; ++i)
    {
        void* placementAddress = reinterpret_cast<char*>(locksMem) + i * sizeof(SegmentLock);
        SegmentLock* segmentLock = reinterpret_cast<SegmentLock*>(placementAddress);
        segmentLock->Init();
    }

    void* statisticsMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int) + (SegmentsNum * sizeof(SegmentLock));
    Statistics* statistics = reinterpret_cast<Statistics*>(statisticsMem);

    statistics->NumOfReaders = 0;
    statistics->NumOfWriters = 0;
    statistics->NumOfRecordsProcessed = 0;
    statistics->TotalReaderTime = 0.0;
    statistics->TotalWritterTime = 0.0;
    statistics->MaxDelay = 0.0;

    void* semMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int) + (SegmentsNum * sizeof(SegmentLock)) + sizeof(Statistics);
    Semaphore* sem = reinterpret_cast<Semaphore*>(semMem);

    sem->Init(1);
}

void Server::DestroyCS()
{
    void* locksMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int);

    // Destroy SegmentLock instances within the shared memory
    for (int i = 0; i < SegmentsNum; ++i)
    {
        void* placementAddress = reinterpret_cast<char*>(locksMem) + i * sizeof(SegmentLock);
        SegmentLock* segmentLock = reinterpret_cast<SegmentLock*>(placementAddress);
        segmentLock->Destroy();
    }

    void* statisticsSemMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int) + (SegmentsNum * sizeof(SegmentLock)) + sizeof(Statistics);
    Semaphore* sem = reinterpret_cast<Semaphore*>(statisticsSemMem);

    // Destroy Statistics semaphore
    sem->Destroy();

    munmap(MappedMemory, TotalSizeCS);
    close(SegmentShmFD);
    shm_unlink(ShmSegmentName);
}

void Server::CreateAndWaitClients(int numOfReaders, int numOfWriters)
{
    pid_t* readersPids = (pid_t*)malloc(numOfReaders * sizeof(pid_t));
    pid_t* writersPids = (pid_t*)malloc(numOfWriters * sizeof(pid_t));
    int readerCount = 0;
    int writerCount = 0;

    // Lambda to generate a random integer between min and max
    auto GenerateRandomInt = [this](int min, int max) -> int 
    {
        int randNum = rand_r(&Seed);
        return min + (randNum % (max - min + 1)); // Scale to the desired range
    };

    // Lambda to sleep for a random duration between 10 to 100 milliseconds
    auto RandomSleep = [&]()
    {
        int sleepTime = GenerateRandomInt(10000, 100000);
        usleep(sleepTime);
    };

    auto MinInt = [](int a, int b) -> int
    {
        return (a < b) ? a : b;
    };

    char readerFilename[32];
    char writerFilename[32];
    strcpy(readerFilename, "./Reader");
    strcpy(writerFilename, "./Writer");

    auto CreateProcesses = [&](int numOfProcesses, char* processFilename)
    {
        for (int i = 0; i < numOfProcesses; ++i)
        {
            RandomSleep();
            
            pid_t pid = fork();
            if (pid == 0)
            {
                char idStr[32];
                char valueStr[32];
                char waitStr[32];

                int recordsNum = Editor.NumOfRecords;
                if (strcmp(processFilename, readerFilename) == 0)
                {
                    int startId = GenerateRandomInt(1, recordsNum);
                    int endId = GenerateRandomInt(startId, MinInt(startId + ReaderMaxRange, recordsNum));
                    int waitTime = GenerateRandomInt(1, ReaderWaitingMaxInSec);
                    snprintf(idStr, sizeof(idStr), "%d,%d", startId, endId);
                    snprintf(waitStr, sizeof(waitStr), "%d", waitTime);
                    execlp(processFilename, "Reader", "-f", Filename, "-l", idStr, "-d", waitStr, "-s", ShmSegmentName, (char*)NULL);
                }
                else if (strcmp(processFilename, writerFilename) == 0)
                {
                    int id = GenerateRandomInt(1, recordsNum);
                    int value = GenerateRandomInt(-WriterMaxValue, WriterMaxValue);
                    int waitTime = GenerateRandomInt(1, WriterWaitingMaxInSec);
                    snprintf(idStr, sizeof(idStr), "%d", id);
                    snprintf(valueStr, sizeof(valueStr), "%d", value);
                    snprintf(waitStr, sizeof(waitStr), "%d", waitTime);
                    execlp(processFilename, "Writer", "-f", Filename, "-l", idStr, "-v", valueStr, "-d", waitStr, "-s", ShmSegmentName, (char*)NULL);
                }
                
                cerr << "Exec failed for " << processFilename << endl;
                exit(EXIT_FAILURE);
            }
            else if (pid > 0)
            {
                if (strcmp(processFilename, readerFilename) == 0)
                {
                    readersPids[readerCount++] = pid;
                }
                else if (strcmp(processFilename, writerFilename) == 0)
                {
                    writersPids[writerCount++] = pid;
                }
            }
            else
            {
                cerr << "Fork failed" << endl;
            }
        }
    };

    pid_t pid = fork();
    if (pid == 0) // Child process
    {
        CreateProcesses(numOfReaders, readerFilename);
        for (int i = 0; i < numOfReaders; ++i)
        {
            waitpid(readersPids[i], NULL, 0);
        }
        
        free(readersPids);
        exit(EXIT_SUCCESS);
    }
    else if (pid > 0) // Parent process
    {
        CreateProcesses(numOfWriters, writerFilename);
        for (int i = 0; i < numOfWriters; ++i)
        {
            waitpid(writersPids[i], NULL, 0);
        }
        waitpid(pid, NULL, 0);
    }
    else
    {
        cerr << "Fork failed" << endl;
    }

    free(readersPids);
    free(writersPids);
}

void Server::PrintStatistics()
{
    void* statisticsMem = reinterpret_cast<char*>(MappedMemory) + sizeof(int) + (SegmentsNum * sizeof(SegmentLock));
    Statistics* statistics = reinterpret_cast<Statistics*>(statisticsMem);

    cout << "Number or readers: " << statistics->NumOfReaders << endl;
    double averageReaderTime = statistics->NumOfReaders != 0 ? statistics->TotalReaderTime / statistics->NumOfReaders : 0;
    cout << "Average readers active time: " << averageReaderTime << endl;

    cout << "Number or writers: " << statistics->NumOfWriters << endl;
    double averageWriterTime = statistics->NumOfWriters != 0 ? statistics->TotalWritterTime / statistics->NumOfWriters : 0;
    cout << "Average writers active time: " << averageWriterTime << endl;

    cout << "Max delay: " << statistics->MaxDelay << endl;
    cout << "Number or records processed: " << statistics->NumOfRecordsProcessed << endl;
}