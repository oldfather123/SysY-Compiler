#ifndef GEECEECEE_LOG_HPP
#define GEECEECEE_LOG_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

namespace logger {
namespace fs = std::filesystem;

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

// make cur_log_level inside a function to avoid static var init related problem
inline LogLevel &get_cur_log_level() {
    static LogLevel cur_log_level = LogLevel::ERROR;
    return cur_log_level;
}

inline void set_log_level(LogLevel level) { get_cur_log_level() = level; }

inline LogLevel get_log_level() { return get_cur_log_level(); }

// --- Platform-specific color handling ---
#ifdef _WIN32
namespace detail {
class WinColorManager {
public:
    WinColorManager() : m_hConsole(GetStdHandle(STD_OUTPUT_HANDLE)) {
        if (m_hConsole != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO info;
            if (GetConsoleScreenBufferInfo(m_hConsole, &info)) {
                m_defaultAttributes = info.wAttributes;
            }
        }
    }

    void set_color(LogLevel level) const {
        if (m_hConsole == INVALID_HANDLE_VALUE) return;

        WORD attributes;
        switch (level) {
            case LogLevel::DEBUG:
                attributes = FOREGROUND_BLUE | FOREGROUND_GREEN;  // Cyan
                break;
            case LogLevel::INFO:
                attributes = FOREGROUND_GREEN;
                break;
            case LogLevel::WARN:
                attributes = FOREGROUND_RED | FOREGROUND_GREEN;  // Yellow
                break;
            case LogLevel::ERROR:
                attributes = FOREGROUND_RED;
                break;
            default:
                attributes = m_defaultAttributes;
                break;
        }
        if (level != LogLevel::INFO) {
            attributes |= FOREGROUND_INTENSITY;
        }
        SetConsoleTextAttribute(m_hConsole, attributes);
    }

    void reset_color() const {
        if (m_hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(m_hConsole, m_defaultAttributes);
        }
    }

private:
    HANDLE m_hConsole;
    WORD m_defaultAttributes = 7;  // Default to gray
};

// Global instance to manage colors
inline const WinColorManager win_color_manager;

}  // namespace detail
#else
// For non-Windows systems, we keep the ANSI color codes
namespace color {
constexpr const char *RED = "\033[31m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *RESET = "\033[0m";
}  // namespace color
#endif

inline void noop() {}

template <typename... Args>
inline void log(
    LogLevel level, const char *level_str, const char *color, const char *file, int line, const Args &...args) {
    if (level < get_cur_log_level()) {
        return;
    }

#ifdef _WIN32
    detail::win_color_manager.set_color(level);

    std::cout << "[" << level_str << "] ";
    try {
        auto rel_path = fs::relative(fs::path(file), fs::current_path());
        std::cout << rel_path.string() << ":" << line << " ";
    } catch (...) {
        std::cout << file << ":" << line << " ";
    }
    std::ostringstream oss;
    (oss << ... << args);
    std::cout << oss.str();
    detail::win_color_manager.reset_color();
    std::cout << std::endl;

#else
    using namespace logger::color;
    std::ostringstream oss;
    oss << color;
    oss << "[" << level_str << "] ";
    try {
        auto rel_path = fs::relative(fs::path(file), fs::current_path());
        oss << rel_path.string() << ":" << line << " ";
    } catch (...) {
        oss << file << ":" << line << " ";
    }
    (oss << ... << args);
    oss << RESET << std::endl;
    std::cout << oss.str();
#endif
}

#ifdef ENABLE_LOG
#ifdef _WIN32
#define DEBUG(...) log(logger::LogLevel::DEBUG, "DEBUG", nullptr, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) log(logger::LogLevel::INFO, "INFO ", nullptr, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...) log(logger::LogLevel::WARN, "WARN ", nullptr, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) log(logger::LogLevel::ERROR, "ERROR", nullptr, __FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUG(...) log(logger::LogLevel::DEBUG, "DEBUG", logger::color::CYAN, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) log(logger::LogLevel::INFO, "INFO ", logger::color::GREEN, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...) log(logger::LogLevel::WARN, "WARN ", logger::color::YELLOW, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) log(logger::LogLevel::ERROR, "ERROR", logger::color::RED, __FILE__, __LINE__, __VA_ARGS__)
#endif
#else
#define DEBUG(...) noop()
#define INFO(...) noop()
#define WARN(...) noop()
#define ERROR(...) noop()
#endif

inline const auto LAUNCH_TIME = std::chrono::system_clock::now();
const fs::path LOG_PATH = fs::current_path() / "log" / []() {
    auto time_t_val = std::chrono::system_clock::to_time_t(LAUNCH_TIME);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}();

inline std::string log_to_file(std::string_view file_name, std::string_view content) {
    auto log_file_path = LOG_PATH / file_name;
    // Create parent directories if they don't exist
    std::filesystem::create_directories(log_file_path.parent_path());

    // Open and write to file
    std::ofstream ofs(log_file_path, std::ios::app | std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file_path.string());
    }

    // Write content to file
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!ofs) {
        throw std::runtime_error("Failed to write to log file: " + log_file_path.string());
    }

    try {
        return std::filesystem::relative(log_file_path, std::filesystem::current_path()).string();
    } catch (...) {
        return log_file_path.string();
    }
}

}  // namespace logger

#endif  // GEECEECEE_LOG_HPP
