#include <iostream>
#include <cstdio>
#include <cstring>
#include "Server.h"

using namespace std;

bool copyFile(const char* sourceFileName, const char* destFileName);

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        cerr << "Wrong number of arguments" << endl;
        return EXIT_FAILURE;
    }

    char* sourceFileName = argv[1];
    int maxNumOfSegments = atoi(argv[2]);
    int readersNum = atoi(argv[3]);
    int writersNum = atoi(argv[4]);
    
    const char* localFileName = "LocalAccounts.bin";

    // Create a local copy of the file so initial is not changed
    if (!copyFile(sourceFileName, localFileName))
    {
        cerr << "Failed to create a local copy of the file" << endl;
        return EXIT_FAILURE;
    }

    Server server(localFileName, maxNumOfSegments);
    server.Init();
    server.CreateAndWaitClients(readersNum, writersNum);
    server.PrintStatistics();

    return EXIT_SUCCESS;
}

bool copyFile(const char* sourceFileName, const char* destFileName)
{
    FILE* source = fopen(sourceFileName, "rb");
    if (!source) { return false; }

    FILE* dest = fopen(destFileName, "wb");
    if (!dest)
    { 
        fclose(source);
        return false; 
    }

    // Buffer to hold file data during copy
    char buffer[4096];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
    {
        fwrite(buffer, 1, bytesRead, dest);
    }

    fclose(source);
    fclose(dest);
    return true;
}
