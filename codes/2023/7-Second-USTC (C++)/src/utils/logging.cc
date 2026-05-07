#include "logging.hh"

void LogWriter::operator<(const LogStream &stream) {
    std::ostringstream msg;
    msg << stream.sstream_->rdbuf();
    output_log(msg);
}

void LogWriter::output_log(const std::ostringstream &msg) {
    if (static_cast<unsigned int>(log_level_) >= env_log_level_) {
        std::cout << "[" << level2string(log_level_) << "] "
                  << "(" << location_.file_ << ":" << location_.line_ << "L, " << location_.func_ << ") "
                  << msg.str().c_str()
                  << std::endl;
    }
    if(log_level_ == LogLevel::ERROR) 
        std::abort();
}

std::string level2string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "\033[94mINFO\033[0m";
        case LogLevel::WARNING:
            return "\033[93;1mWARNING\033[0m";
        case LogLevel::ERROR:
            return "\033[31;1mERROR\033[0m";
    }
    return "ERROR";
}

std::string get_short_name(const char *file_path) {
    std::string short_file_path = file_path;
    int index = short_file_path.find_last_of('/');

    return short_file_path.substr(index + 1);
}