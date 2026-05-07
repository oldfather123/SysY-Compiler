#include "sysY_driver.hh"

sysY_driver::sysY_driver () : trace_scanning (false), trace_parsing (false){}

sysY_driver::~sysY_driver () {}

ASTCompUnit* sysY_driver::parse (const std::string &f) {
    file = f;
    scan_begin ();
    parser = new yy::sysY_parser(*this);
    parser->set_debug_level (trace_parsing);
    parser->parse ();
    scan_end ();
    return this->root;
}


void sysY_driver::error (const yy::location& l, const std::string& m) {
    std::cout << l << ": " << m;
}

void sysY_driver::error (const std::string& m) {
    std::cerr << m << std::endl;
    std::cout << ": " << m;
}



void sysY_driver::scan_begin() {
    lexer = new sysY_flexlexer();
    lexer->set_debug( trace_scanning );

    instream.open(file);

    if( instream.good() ) {
        lexer->switch_streams(&instream, 0);
    } else if( file == "-" ) {
        lexer->switch_streams(&std::cin, 0);
    } else {
        error("Cannot open file '" + file + "'.");
        exit(EXIT_FAILURE);
    }
}


void sysY_driver::scan_end () {
    instream.close();
}