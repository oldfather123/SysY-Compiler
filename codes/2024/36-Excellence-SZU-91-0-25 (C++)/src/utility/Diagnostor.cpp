#include "../utility/Diagnostor.h"
#include "../utility/FileManager.h"
#include "../utility/Product.h"

void WhoAmI(string name)
{
    if(WHO)cout<<name<<endl;
}

void Debug(string debug_message)
{
    if(DEBUG)
        cout<<"[Debug] # "<<debug_message<<" #"<<endl;
}



void Location(Cursor location)
{
    //@Todo:Error color
    cout<<" #Line "<<location.line+1<<", Column "<<location.column+1<<endl;
    
    //Print source
    cout<<source[location.line];
    //Print blank
    for(int i=0;i<location.column;i++)
    {
        if(source[location.line][i]=='\t')cout<<"\t";
        else cout<<" ";
    }
    //Print mark
    cout<<"^"<<endl;
}

void Exit(int exit_code)
{
    file_manager.Close();
    exit(exit_code);
}



void Error(string error_message)
{
    cout<<"Error: "<<error_message<<endl;
    Exit(-1);
}

void Error(string error_message,Cursor location)
{
    cout<<"Error: "<<error_message;
    Location(location);
    Exit(-1);
}

void FileError(string error_message)
{
    cout<<"File Error: "<<error_message<<endl;
    Exit(-2);
}

void ScanError(string error_message,Cursor location)
{
    cout<<"Scan Error: "<<error_message;
    Location(location);
    Exit(-3);
}

void ParseError(string error_message,Cursor location)
{
    cout<<"Parse Error: "<<error_message;
    Location(location);
    Exit(-4);
}

void SymbolError(string error_message)
{
    cout<<"Symbol Error: "<<error_message;
    Exit(-5);
}

void SymbolError(string error_message,Cursor location)
{
    cout<<"Symbol Error: "<<error_message;
    Location(location);
    Exit(-5);
}

void TypeError(string error_message)
{
    cout<<"Type Error: "<<error_message<<endl;
    Exit(-6);
}

void TypeError(string error_message,Cursor location)
{
    cout<<"Type Error: "<<error_message;
    Location(location);
    Exit(-6);
}

void InitialError(string error_message,Cursor location)
{
    cout<<"Initial Error: "<<error_message;
    Location(location);
    Exit(-7);
}

void AssignError(string error_message,Cursor location)
{
    cout<<"Assign Error: "<<error_message;
    Location(location);
    Exit(-8);
}



void CompilerError(string error_message)
{
    cout<<"xxx Compiler Error: "<<error_message<<endl;
    Exit(-9);
}
