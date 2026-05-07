#include "../utility/FileManager.h"
#include "../utility/Diagnostor.h"
#include "../utility/Product.h"

FileManager file_manager;

void FileManager::Initialize(string input_file_name,string output_file_name)
{
    this->input_file_name=input_file_name;
    this->output_file_name=output_file_name;
}

void FileManager::Open()
{
    input_file.open(input_file_name,ios::in);
    if(!input_file.is_open())
        FileError("Open the input file error!");

    if(output_file_name!="")
    {
        output_file.open(output_file_name,ios::out);
        if(!output_file.is_open())
            FileError("Open the output file error!");
    }

}

void FileManager::Load()
{
    source.clear();
    string buffer;

    //Read line by line
    while(getline(input_file,buffer))
    {
        buffer+='\n';
        source.push_back(buffer);
    }
}

void FileManager::Write(string code)
{
    output_file<<code;
}

void FileManager::Close()
{
    input_file.close();
}
