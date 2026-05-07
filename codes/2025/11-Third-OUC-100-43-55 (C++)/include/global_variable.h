#ifndef IR_GLOBAL_VARIABLE_H
#define IR_GLOBAL_VARIABLE_H

#include "constant.h"
#include <memory>
#include <string>
#include <vector>

namespace ir {

class GlobalVariable : public Value {
private:
  std::string _name;
  bool _isConst;
  std::unique_ptr<Constant> _value;

public:
  GlobalVariable(std::unique_ptr<Type> type, std::string name, bool isConst,
                 std::unique_ptr<Constant> value);

  bool isConst() const { return _isConst; }
  ValueKind getValueKind() const override { return ValueKind::Global; }

  Constant *getValue() const { return _value.get(); }

  bool isSingle() const;
  bool isInBss() const;

  int getInt() const;
  int getInt(size_t index) const;

  float getFloat() const;
  float getFloat(size_t index) const;

  std::string getRawName() const;
  std::string getName() const;

  std::string str() const override;
};

} // namespace ir

#endif // IR_GLOBAL_VARIABLE_HPP