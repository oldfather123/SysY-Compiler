#pragma once

#include "../utility/System.h"

class FileManager
{
private:
    string input_file_name;
    string output_file_name;

    ifstream input_file;
    ofstream output_file;

public:
    void Initialize(string input_file_name,string output_file_name);
    void Open();
    void Load();
    void Write(string code);
    void Close();
};

extern FileManager file_manager;
