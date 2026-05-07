#include "error.hpp"

using std::string;
using std::string_view;

Exception::Exception(const string_view &__error) : _error(__error) {
}

const char *Exception::what() const noexcept {
  return _error.cbegin();
};

AssertError::AssertError(const string_view &__error) : Exception(string_view("AssertError: " + string(__error))) {
}

RuntimeError::RuntimeError(const string_view &__error) : Exception(string_view("RuntimeError: " + string(__error))) {
}

LogicError::LogicError(const string_view &__error) : Exception(string_view("LogicError: " + string(__error))) {
}

AllocaError::AllocaError(const string_view &__error) : Exception(string_view("AllocaError: " + string(__error))) {
}

SyntaxError::SyntaxError(const string_view &__error) : Exception(string_view("SyntaxError: " + string(__error))) {
}

AsmError::AsmError(const string_view &__error) : Exception(string_view("AsmError: " + string(__error))) {
}

InvalidType::InvalidType() : SyntaxError("invalid type"){};

InvalidOperator::InvalidOperator() : SyntaxError("invalid operator"){};

InvalidIndex::InvalidIndex() : RuntimeError("invalid index"){};

InvalidDim::InvalidDim() : SyntaxError("invalid dim"){};

InvalidRealParam::InvalidRealParam() : RuntimeError("invalid real param"){};
