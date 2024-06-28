#include "RecordEditor.h"
#include <iostream>
#include <cstring>
#include <cstdio>

using namespace std;

RecordEditor::RecordEditor(const char* fileName)
{
    FileName = new char[strlen(fileName) + 1];
    strcpy(FileName, fileName);
    NumOfRecords = GetNumberOfRecords();
}

RecordEditor::~RecordEditor()
{
    delete[] FileName;
}

int RecordEditor::GetNumberOfRecords()
{
    FILE* file = fopen(FileName, "rb");
    if (!file) { return -1; }

    fseek(file, 0, SEEK_END);
    int count = ftell(file) / RecordSize;
    fclose(file);
    return count;
}

Account RecordEditor::ReadRecordById(int custId)
{
    FILE* file = fopen(FileName, "rb");
    if (!file || custId < 1 || custId > NumOfRecords)
    {
        cerr << "Wrong file name or Id out of range" << endl;
        cerr << "Filename: " << FileName << " Id: " << custId << endl;
        Account emptyAccount = {};
        return emptyAccount;
    }

    fseek(file, (custId - 1) * RecordSize, SEEK_SET);
    Account record;
    fread(&record, RecordSize, 1, file);
    fclose(file);
    return record;
}

void RecordEditor::WriteRecordById(int custId, const Account& record)
{
    FILE* file = fopen(FileName, "r+b");
    if (!file || custId < 1 || custId > NumOfRecords)
    {
        cerr << "Wrong file name or Id out of range" << endl;
        cerr << "Filename: " << FileName << " Id: " << custId << endl;
        return;
    }

    fseek(file, (custId - 1) * RecordSize, SEEK_SET);
    fwrite(&record, RecordSize, 1, file);
    fclose(file);
}
