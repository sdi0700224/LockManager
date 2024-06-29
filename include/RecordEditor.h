#pragma once
#include <fstream>

struct Account
{
    int CustId;
    char LastName[20];
    char FirstName[20];
    int Balance;
};

class RecordEditor
{
private:
    char* FileName;
    static const int RecordSize = sizeof(Account);
    int GetNumberOfRecords();

public:
    int NumOfRecords;

    RecordEditor(const char* fileName);
    ~RecordEditor();

    Account ReadRecordById(int custId);
    void WriteRecordById(int custId, const Account& record);
};
