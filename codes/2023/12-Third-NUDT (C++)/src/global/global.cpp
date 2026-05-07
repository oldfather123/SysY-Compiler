#include "global.hpp"

using std::string;
using std::string_view;

Configuration::Configuration() : _os(&std::cerr), _pos(nullptr), _log_level(DEBUG), _plog_level(DEBUG) {
}

Configuration::Configuration(LogLevel log_level, std::ostream &os)
    : _os(&os), _pos(nullptr), _log_level(log_level), _plog_level(DEBUG) {
}

Configuration::~Configuration() {
}

std::ostream &Configuration::os() {
  return *_os;
}

LogLevel Configuration::logLevel() {
  return this->_log_level;
}

std::string Configuration::filename() {
  return _filename;
}

void Configuration::setFilename(const std::string &filename) {
  _filename = filename;
}

void Configuration::saveOstream() {
  _pos = _os;
}

void Configuration::loadOstream() {
  _os = _pos;
}

void Configuration::setOstream(std::ostream &os) {
  _os = &os;
};

void Configuration::saveLogLevel() {
  _plog_level = _log_level;
}

void Configuration::loadLogLevel() {
  _log_level = _plog_level;
}

void Configuration::setLogLevel(LogLevel log_level) {
  this->_log_level = log_level;
}

Configuration configuration = Configuration();

Logger::Logger() : _log_level(DEBUG) {
}

Logger::Logger(LogLevel log_level) : _log_level(log_level) {
}

Logger::~Logger() {
}

Logger debug = Logger(DEBUG);
Logger info = Logger(INFO);
Logger warn = Logger(WARN);
Logger error = Logger(ERROR);
Logger fatal = Logger(FATAL);

using IR::Type;

FunctionInterface builtin_functions[] = {
    {"getint", Type::function_t(Type::i32_t(), {})},
    {"getch", Type::function_t(Type::i32_t(), {})},
    {"getfloat", Type::function_t(Type::float_t(), {})},
    {"getarray", Type::function_t(Type::i32_t(), {Type::pointer_t(Type::i32_t())})},
    {"getfarray", Type::function_t(Type::i32_t(), {Type::pointer_t(Type::float_t())})},
    {"putint", Type::function_t(Type::void_t(), {Type::i32_t()})},
    {"putch", Type::function_t(Type::void_t(), {Type::i32_t()})},
    {"putfloat", Type::function_t(Type::void_t(), {Type::float_t()})},
    {"putarray", Type::function_t(Type::void_t(), {Type::i32_t(), Type::pointer_t(Type::i32_t())})},
    {"putfarray", Type::function_t(Type::void_t(), {Type::i32_t(), Type::pointer_t(Type::float_t())})},
    {"putf", Type::function_t(Type::void_t(), {Type::pointer_t(Type::char_t()), Type::dynamic_t()})},
    {"starttime", Type::function_t(Type::void_t(), {})},
    {"stoptime", Type::function_t(Type::void_t(), {})},
};

std::set<string> builtin_function_names({
    "getint",
    "getch",
    "getfloat",
    "getarray",
    "getfarray",
    "putint",
    "putch",
    "putfloat",
    "putarray",
    "putfarray",
    "putf",
    "starttime",
    "stoptime",
});
