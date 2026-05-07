#ifndef ARG_PARSER
#define ARG_PARSER

#include <filesystem>
#include <string>
#include <vector>
struct Config {
    std::string exe_name; // compiler exe name
    std::filesystem::path input_file;
    std::filesystem::path output_file;

    bool emitllvm{false};
    bool emitasm{false};
    bool optimise{false};
	
	bool show_passes{false};
    bool rookie_allocator{false};

    Config(int argc, char **argv) : argc(argc), argv(argv) {
        parse_cmd_line();
        check();
    }

  private:
    int argc{-1};
    char **argv{nullptr};

    void parse_cmd_line();
    void check();
    // print helper infomation and exit
    void print_help() const;
    void print_err(const std::string &msg) const;
};


#endif
