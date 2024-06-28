#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/times.h>
#include <ctime>
#include <sys/time.h>
#include "RecordEditor.h"
#include "SegmentLock.h"
#include "Server.h"
#define MAX_NAME_LENGTH 256

using namespace std;

int GetBlockNumber(int custId, RecordEditor& editor, int numOfSegments);
void SafeLogToFile(int fd, const char* data);

int main(int argc, char *argv[])
{
    char filename[MAX_NAME_LENGTH] = {0};
    char shmid[MAX_NAME_LENGTH] = {0};
    int startId = 0, endId = 0, time = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            strncpy(filename, argv[++i], MAX_NAME_LENGTH - 1);
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            char *token = strtok(argv[++i], ",");
            startId = atoi(token);
            token = strtok(NULL, ",");
            endId = token ? atoi(token) : startId;
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            time = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            strncpy(shmid, argv[++i], MAX_NAME_LENGTH - 1);
        }
    }

    RecordEditor editor(filename);

    if (startId < 1 || startId > endId || endId > editor.NumOfRecords)
    {
        cerr << "Wrong range--> start: " << startId << " end: " << endId << " Actual num of records: " << editor.NumOfRecords << endl;
        return EXIT_FAILURE; 
    }

    // Open shared memory
    int fd = shm_open(shmid, O_RDWR, 0666);
    if (fd == -1)
    {
        cerr << "Failed to open shared memory segment: " << shmid << endl;
        return EXIT_FAILURE;
    }

    int* initialMapping = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (initialMapping == MAP_FAILED)
    {
        cerr << "Initial mapping failed" << endl;
        close(fd);
        return 1;
    }

    int segmentsNum = *initialMapping;

    // Unmap the initial mapping
    munmap(initialMapping, sizeof(int));

    // Map again with the full size
    int segmentSize = sizeof(int) + (segmentsNum * sizeof(SegmentLock)) + sizeof(Server::Statistics) + sizeof(Semaphore);
    void* fullMapping = mmap(NULL, segmentSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fullMapping == MAP_FAILED)
    {
        cerr << "Full mapping failed" << endl;
        close(fd);
        return 1;
    }

    int startingBlockIndex = GetBlockNumber(startId, editor, segmentsNum) - 1;
    int endingBlockIndex = GetBlockNumber(endId, editor, segmentsNum) - 1;
    if (startingBlockIndex < 0 || endingBlockIndex < 0) { return EXIT_FAILURE; }

    void* segmentMem = reinterpret_cast<char*>(fullMapping) + sizeof(int);

    char buffer[MAX_NAME_LENGTH];
    int logFd = open("Log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (logFd == -1)
    {
        cerr << "Failed to open file" << endl;
        return EXIT_FAILURE;
    }

    double t1, t2, t3;
    struct tms tb1, tb2, tb3;
    double ticspersec;
    ticspersec = (double)sysconf(_SC_CLK_TCK);
    t1 = (double)times(&tb1);

    for (int i = startingBlockIndex; i <= endingBlockIndex; i++)
    {
        void* placementAddress = reinterpret_cast<char*>(segmentMem) + i * sizeof(SegmentLock);
        SegmentLock* segmentLock = reinterpret_cast<SegmentLock*>(placementAddress);
        segmentLock->ReaderLock();
    }

    t2 = (double)times(&tb2);
    double realWaitTime = (t2 - t1) / ticspersec;
    
    int sum = 0;
    int count = endId - startId + 1;
    for (int i = startId; i <= endId; i++)
    {
        Account account = editor.ReadRecordById(i);

        cout << "Id: " << account.CustId << " Name: " << account.FirstName << " Lastname: " << account.LastName << " Balance: " << account.Balance << endl << endl;
        snprintf(buffer, sizeof(buffer), "Id: %d Name: %s Lastname: %s Balance: %d\n\n", account.CustId, account.FirstName, account.LastName, account.Balance);
        SafeLogToFile(logFd, buffer);

        sum += account.Balance;
    }
    double average = (double)sum / count;

    sleep(time);

    cout << "Average: " << average << endl << endl;
    snprintf(buffer, sizeof(buffer), "Average: %f\n", average);
    SafeLogToFile(logFd, buffer);

    for (int i = endingBlockIndex; i >= startingBlockIndex; i--)
    {
        void* placementAddress = reinterpret_cast<char*>(segmentMem) + i * sizeof(SegmentLock);
        SegmentLock* segmentLock = reinterpret_cast<SegmentLock*>(placementAddress);
        segmentLock->ReaderUnlock();
    }

    t3 = (double)times(&tb3);
    double realActiveTime = (t3 - t2) / ticspersec;

    void* statisticsMem = reinterpret_cast<char*>(fullMapping) + sizeof(int) + (segmentsNum * sizeof(SegmentLock));
    Server::Statistics* statistics = reinterpret_cast<Server::Statistics*>(statisticsMem);
    
    void* statisticsSemMem = reinterpret_cast<char*>(fullMapping) + sizeof(int) + (segmentsNum * sizeof(SegmentLock)) + sizeof(Server::Statistics);
    Semaphore* sem = reinterpret_cast<Semaphore*>(statisticsSemMem);

    sem->Wait();
    statistics->NumOfReaders++;
    statistics->NumOfRecordsProcessed += count;
    statistics->TotalReaderTime += realActiveTime;
    statistics->MaxDelay = realWaitTime > statistics->MaxDelay ? realWaitTime : statistics->MaxDelay;
    sem->Release();

    // Cleanup
    munmap(fullMapping, segmentSize);
    close(fd);
    close(logFd);

    return EXIT_SUCCESS;
}

int GetBlockNumber(int custId, RecordEditor& editor, int numOfSegments)
{
    int numRecords = editor.NumOfRecords;
    if (numRecords <= 0)
    {
        cerr << "Reader did not find accounts in file" << endl;
        return -1;
    }
    
    int recordsPerBlock = numRecords / numOfSegments;
    return ((custId - 1) / recordsPerBlock) + 1;
}

void SafeLogToFile(int fd, const char* data)
{
    // Get current time with milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm *tm_now = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_now);
    int millis = tv.tv_usec / 1000;

    pid_t pid = getpid();

    // Prepare the message with time prefix and process ID
    char message[MAX_NAME_LENGTH + 128];
    int length = snprintf(message, sizeof(message), "%s.%03d - Reader (PID: %d) --> %s", time_str, millis, pid, data);

    if (length > 0 && write(fd, message, length) == -1)
    {
        cerr << "Write failed" << endl;
    }
}