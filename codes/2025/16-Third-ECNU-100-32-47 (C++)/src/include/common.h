#ifndef AAAC_COMMON_H
#define AAAC_COMMON_H

#include <ostream>
#pragma once

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <list>

#define DEF_LIST(type) using type##List = std::list<std::shared_ptr<type>>;
namespace aaac {
template <typename Derived> struct Createable {
  template <typename... Args>
  static std::shared_ptr<Derived> create(Args &&...args) {
    return std::make_shared<Derived>(std::forward<Args>(args)...);
  }
};
namespace backend{
class Insn;
struct Function;
class BasicBlock;
DEF_LIST(Insn);
}
namespace sym{
class Var;
DEF_LIST(Var);
}
} // namespace aaac

#define ANSI_FG_BLACK "\033[1;30m"
#define ANSI_FG_RED "\033[1;31m"
#define ANSI_FG_GREEN "\033[1;32m"
#define ANSI_FG_YELLOW "\033[1;33m"
#define ANSI_FG_BLUE "\033[1;34m"
#define ANSI_NONE "\033[0m"

#ifdef WRITE_TO_FILE
#define LOG_WRITE(format, ...)                                                 \
  do {                                                                         \
    char timestamp[20];                                                        \
    time_t now = time(NULL);                                                   \
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));  \
    char filename[32];                                                         \
    snprintf(filename, sizeof(filename), "%s.log", timestamp);                 \
    FILE *fp = fopen(filename, "a");                                           \
    if (fp) {                                                                  \
      fprintf(fp, "[%s] ", timestamp);                                         \
      fprintf(fp, format, ##__VA_ARGS__);                                      \
      fprintf(fp, "\n");                                                       \
      fflush(fp);                                                              \
      fclose(fp);                                                              \
    }                                                                          \
  } while (0)

#else
#define LOG_WRITE(...)
#endif

#define ANSI_FMT(str, fmt) fmt str ANSI_NONE

#define LOG(color, format, ...)                                                \
  do {                                                                         \
    printf(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_##color) "\n", __FILE__,     \
           __LINE__, __func__, ##__VA_ARGS__);                                 \
    LOG_WRITE("[%s:%d %s] " format "\n", __FILE__, __LINE__, __func__,         \
              ##__VA_ARGS__);                                                  \
  } while (0)

#define LOG_ERROR(format, ...) LOG(RED, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...) LOG(BLUE, format, ##__VA_ARGS__);
#define LOG_SUCC(format, ...) LOG(GREEN, format, ##__VA_ARGS__);

#define Assert(cond, format, ...)                                              \
  do {                                                                         \
    if (!(cond)) {                                                             \
      LOG_ERROR("[Assert Failed: %s] [INFO]: " format, #cond, ##__VA_ARGS__);  \
      assert((cond));                                                          \
    }                                                                          \
  } while (0)

#define panic(format, ...) Assert(0, format, ##__VA_ARGS__)
#define TODO() panic("Please implement me")

class DebugDumpImpl{
  public:
    virtual std::ostream &dump(std::ostream &os)const{return os;};
    virtual ~DebugDumpImpl()=default;
};

inline std::ostream& operator<<(std::ostream& os,const DebugDumpImpl& obj){
  return obj.dump(os);
}

#endif // AAAC_COMMON_H
