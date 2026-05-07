#pragma once
#include <memory>

#include <cassert>
#include <cstdlib>
#include <cxxabi.h>
#include <iostream>
#include <stdexcept>
#include <string>

// color
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
// reset
#define RESET "\033[0m"

// NOLINTBEGIN
//  Debug logging macro
#ifdef DEBUG
#define DEBUG_LOG(msg) std::cerr << YELLOW "[DEBUG] " RESET << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif

// Error logging macro
#define ERROR_LOG(msg) std::cerr << RED "[ERROR] " RESET << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl

// Assert macro with message
#define ASSERT(condition, message)                                                                                      \
    do {                                                                                                                \
        if(!(condition)) {                                                                                              \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " << __LINE__ << ": " << message \
                      << std::endl;                                                                                     \
            std::terminate();                                                                                           \
        }                                                                                                               \
    } while(false)

// Function to check if a pointer is null
template <typename T>
inline void checkNull(T* ptr, const std::string& message) {
    if(ptr == nullptr) {
        throw std::runtime_error(message);
    }
}

// Safe downcasting function
template <typename Derived, typename Base>
inline Derived* safeDynamicCast(Base* ptr) {
    ASSERT(ptr != nullptr, "Attempting to cast null pointer");
    Derived* derived = dynamic_cast<Derived*>(ptr);
    ASSERT(derived != nullptr, "Dynamic cast failed");
    return derived;
}

// Function to get the name of a type
template <typename T>
std::string getTypeName() {
#ifdef __GNUG__
    int status;
    char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string result(realname);
    free(realname);
    return result;
#else
    return typeid(T).name();
#endif
}

// Macro for unreachable code
#define UNREACHABLE()                          \
    do {                                       \
        ERROR_LOG("Unreachable code reached"); \
        std::abort();                          \
    } while(false)

// Macro for deprecated functions
#define DEPRECATED(func) __attribute__((deprecated)) func

// Macro for marking a function as nodiscard
#define NODISCARD [[nodiscard]]

// Function to safely convert between numeric types
template <typename To, typename From>
To safeNumericCast(From value) {
    To result = static_cast<To>(value);
    ASSERT(static_cast<From>(result) == value, "Numeric cast would result in data loss");
    return result;
}
// NOLINTEND