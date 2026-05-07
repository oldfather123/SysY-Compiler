#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <cstdlib>
#include <string>

template<typename T>
std::string getTypeName() {
    const char* typeidName = typeid(T).name();
    int status;

    std::unique_ptr<char, void(*)(void*)> demangled(
        abi::__cxa_demangle(typeidName, nullptr, nullptr, &status),
        std::free
    );

    return (status == 0) ? demangled.get() : typeidName;
}
