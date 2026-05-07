#ifndef IR_REGISTER_H
#define IR_REGISTER_H

#define FMT_HEADER_ONLY
#include "core.h"
#include "type.h"
#include <stdexcept>

namespace ir {

class Reg {
private:
  BasicType *_type;

protected:
  explicit Reg(BasicType *type) : _type(type) {}

public:
  virtual ~Reg() = default;
  virtual std::string str() const = 0;
  BasicType *getRegType() const { return _type; }
};

class VReg : public Reg {
private:
  int _id;

public:
  VReg(BasicType *type, int id) : Reg(type), _id(id) {}
  std::string str() const override {
    switch (getRegType()->getBasicKind()) {
    case BasicKind::I32:
      return fmt::format("$i{}", _id);
    case BasicKind::F32:
      return fmt::format("$f{}", _id);
    default:
      throw std::runtime_error("Invalid type in VReg");
    }
  }
};

} // namespace ir

#endif
