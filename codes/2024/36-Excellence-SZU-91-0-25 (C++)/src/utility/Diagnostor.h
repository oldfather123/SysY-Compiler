#pragma once

#include "../utility/System.h"
#include "../utility/Cursor.h"

#define WHO 0
#define DEBUG 1
void WhoAmI(string name);
void Debug(string debug_message);

void Error(string error_message);
void Error(string error_message,Cursor location);

void FileError(string error_message);
void ScanError(string error_message,Cursor location);
void ParseError(string error_message,Cursor location);
void SymbolError(string error_message);
void SymbolError(string error_message,Cursor location);
void TypeError(string error_message);
void TypeError(string error_message,Cursor location);
void InitialError(string error_message,Cursor location);
void AssignError(string error_message,Cursor location);

void CompilerError(string error_message);