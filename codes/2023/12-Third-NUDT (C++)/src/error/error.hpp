#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <bits/stdc++.h>

class Exception : public std::exception {
protected:
  std::string_view _error;

public:
  Exception(const std::string_view &__error);
  virtual ~Exception() = default;
  const char *what() const noexcept override;
};

/// @brief AssertError
class AssertError : public ::Exception {
public:
  AssertError(const std::string_view &__error);
  virtual ~AssertError() = default;
};

class Unreachable : public ::AssertError {
public:
  Unreachable() : AssertError("unreachable") {
  }
};

/// @brief AllocaError
class AllocaError : public ::Exception {
public:
  AllocaError(const std::string_view &__error);
  virtual ~AllocaError() = default;
};

/// @brief RuntimeError
class RuntimeError : public ::Exception {
public:
  RuntimeError(const std::string_view &__error);
  virtual ~RuntimeError() = default;
};

class InvalidToPointer : public ::RuntimeError {
public:
  InvalidToPointer() : RuntimeError("invalid to pointer") {
  }
};

class InvalidRetType : public ::RuntimeError {
public:
  InvalidRetType() : RuntimeError("invalid ret type") {
  }
};

class InvalidParamTypes : public ::RuntimeError {
public:
  InvalidParamTypes() : RuntimeError("invalid ret type") {
  }
};

class InvalidIndex : public ::RuntimeError {
public:
  InvalidIndex();
};

class InvalidRealParam : public ::RuntimeError {
public:
  InvalidRealParam();
};

class Overflow : public ::RuntimeError {
public:
  Overflow() : RuntimeError("overflow") {
  }
};

class Downflow : public ::RuntimeError {
public:
  Downflow() : RuntimeError("downflow") {
  }
};

/// @brief LogicError
class LogicError : public ::Exception {
public:
  LogicError(const std::string_view &__error);
  virtual ~LogicError() = default;
};

class InvalidDivide : public ::LogicError {
public:
  InvalidDivide() : LogicError("divide 0") {
  }
};

class InvalidRem : public ::LogicError {
public:
  InvalidRem() : LogicError("float rem") {
  }
};

/// @brief SyntaxError
class SyntaxError : public ::Exception {
public:
  SyntaxError(const std::string_view &__error);
  virtual ~SyntaxError() = default;
};

class InvalidType : public ::SyntaxError {
public:
  InvalidType();
};

class InvalidOperator : public ::SyntaxError {
public:
  InvalidOperator();
};

class InvalidDim : public ::SyntaxError {
public:
  InvalidDim();
};

class InvalidInit : public ::SyntaxError {
public:
  InvalidInit() : SyntaxError("invalid init") {
  }
};

class InvalidFuncDef : public ::SyntaxError {
public:
  InvalidFuncDef() : SyntaxError("invalid func def") {
  }
};

class InvalidVarDef : public ::SyntaxError {
public:
  InvalidVarDef() : SyntaxError("invalid var def") {
  }
};

class InvalidFormParam : public ::SyntaxError {
public:
  InvalidFormParam() : SyntaxError("invalid form param") {
  }
};

class NoMainFunc : public ::SyntaxError {
public:
  NoMainFunc() : SyntaxError("no main func") {
  }
};

class InvalidMainFuncType : public ::SyntaxError {
public:
  InvalidMainFuncType() : SyntaxError("invalid main func type") {
  }
};

/// @brief LogicError
class UnknownVar : public ::LogicError {
public:
  UnknownVar() : LogicError("unknown var") {
  }
};

class UnknownFunc : public ::LogicError {
public:
  UnknownFunc() : LogicError("unknown func") {
  }
};

class InvalidDeref : public ::RuntimeError {
public:
  InvalidDeref() : RuntimeError("invalid deref") {
  }
};

class InvalidAssign : public ::SyntaxError {
public:
  InvalidAssign() : SyntaxError("invalid assign") {
  }
};

/// @brief AsmError
class AsmError : public ::Exception {
public:
  AsmError(const std::string_view &__error);
  virtual ~AsmError() = default;
};

class InvalidInst : public ::AsmError {
public:
  InvalidInst() : AsmError("invalid inst") {
  }
};

#endif
