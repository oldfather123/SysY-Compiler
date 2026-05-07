#pragma once

#include <fstream>
#include <map>

#include "sysY_parser.hh"
#include "sysY_flexlexer.hh"
#include "logging.hh"

class sysY_driver {
public:
    sysY_driver();
    virtual ~sysY_driver();

public:
    void scan_begin ();
    void scan_end ();
    ASTCompUnit* parse (const std::string& f);
    void error (const yy::location& l, const std::string& m);
    void error (const std::string& m);

public:


    int result; 

    std::ifstream instream;
    std::string file;
  
    bool trace_scanning;
    bool trace_parsing;
    
    sysY_flexlexer *lexer = nullptr;
    yy::sysY_parser *parser = nullptr;

    ASTCompUnit* root = nullptr;

};