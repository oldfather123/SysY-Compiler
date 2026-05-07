#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__

#include <bits/stdc++.h>

#include "ir.hpp"

using LogLevel = enum LogLevel : size_t { DEBUG, INFO, WARN, ERROR, FATAL };

class Configuration {
private:
  std::string _filename = "";
  std::ostream *_os;
  std::ostream *_pos;
  LogLevel _log_level;
  LogLevel _plog_level;

public:
  Configuration();
  Configuration(LogLevel log_level, std::ostream &os = std::cerr);
  ~Configuration();
  std::ostream &os();
  LogLevel logLevel();
  std::string filename();
  void setFilename(const std::string &filename);
  void saveOstream();
  void loadOstream();
  void setOstream(std::ostream &os);
  void saveLogLevel();
  void loadLogLevel();
  void setLogLevel(LogLevel log_level);
};

class Logger {
private:
  LogLevel _log_level;

public:
  Logger();
  Logger(LogLevel log_level);
  ~Logger();
  template <typename T>
  Logger &operator<<(const T &msg);
};

struct FunctionInterface {
  std::string name;
  IR::Type *type;
};

extern Configuration configuration;
extern Logger debug;
extern Logger info;
extern Logger warn;
extern Logger error;
extern Logger fatal;
#define BUILTIN_FUNCTIONS 13
extern FunctionInterface builtin_functions[BUILTIN_FUNCTIONS];
extern std::set<std::string> builtin_function_names;

template <typename T>
Logger &Logger::operator<<(const T &msg) {
  if (this->_log_level >= configuration.logLevel()) configuration.os() << msg;
  return *this;
};

#endif
