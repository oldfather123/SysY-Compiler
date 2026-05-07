#include "symbol_table.h"

ir::Value *SymbolTable::getItem(const std::string &name) const {
  for (auto item : _table) {
    if (item.find(name) != item.end())
      return item[name];
  }
  throw std::runtime_error("Undefined variable: " + name);
}

std::map<int, ir::Number>
SymbolTable::submap(const std::map<int, ir::Number> &values, int fromKey,
                    int toKey) {
  std::map<int, ir::Number> sub;
  auto lower = values.lower_bound(fromKey);
  auto upper = values.upper_bound(toKey);

  for (; lower != upper; ++lower)
    sub.insert(*lower);

  return sub;
}

std::unique_ptr<ir::Constant>
SymbolTable::fuseConst(std::unique_ptr<ir::Type> type,
                       const std::map<int, ir::Number> &values, int base) {
  if (type->isBasic())
    if (values.find(base) != values.end())
      return std::make_unique<ir::ConstantNumber>(values.at(base));
    else
      return std::make_unique<ir::ConstantNumber>(ir::Number(0));

  if (values.empty())
    return std::make_unique<ir::ConstantZero>(std::move(type));

  auto arrayType = static_cast<ir::ArrayType *>(type.get());
  size_t size = 1;
  for (auto i : arrayType->getDimensions())
    size *= i;

  std::vector<std::unique_ptr<ir::Constant>> array;
  for (size_t i = base; i < base + size; i += size / arrayType->getLength())
    array.push_back(
        fuseConst(arrayType->getElementType()->clone(),
                  submap(values, static_cast<int>(i),
                         static_cast<int>(i + size / arrayType->getLength())),
                  static_cast<int>(i)));
  return std::make_unique<ir::ConstantArray>(std::move(type), std::move(array));
}

ir::Function *SymbolTable::getFunction(const std::string &name) const {
  auto symbol = getItem(name);
  if (auto func = dynamic_cast<ir::Function *>(symbol))
    return func;
  throw std::runtime_error("Undefined function: " + name);
}

std::unique_ptr<ir::Function>
SymbolTable::makeFunction(std::unique_ptr<ir::Type> type,
                          const std::string &name) {
  auto func = std::make_unique<ir::Function>(std::move(type), name);
  _table.back()[name] = func.get();
  return std::move(func);
}

std::unique_ptr<ir::GlobalVariable>
SymbolTable::makeGlobal(bool isConst, std::unique_ptr<ir::Type> type,
                        const std::string &name, ir::Number value) {
  assert(type->isBasic());
  auto basicType = static_cast<ir::BasicType *>(type.get())->getBasicKind();
  switch (basicType) {
  case ir::BasicKind::I32:
    value = ir::Number(value.intValue());
    break;
  case ir::BasicKind::F32:
    value = ir::Number(value.floatValue());
    break;
  default:
    throw std::runtime_error("Unsupported type in makeGlobal");
  }

  auto symbol = std::make_unique<ir::GlobalVariable>(
      std::move(type), name, isConst,
      std::make_unique<ir::ConstantNumber>(value));
  _table.front()[name] = symbol.get();
  return std::move(symbol);
}

std::unique_ptr<ir::GlobalVariable>
SymbolTable::makeGlobal(bool isConst, std::unique_ptr<ir::Type> type,
                        const std::string &name,
                        std::map<int, ir::Number> &values) {
  ir::BasicType *rootType;
  if (type->isArray()) {
    rootType =
        static_cast<ir::ArrayType *>(type.get())->getInnermostElementType();
  } else {
    rootType = static_cast<ir::BasicType *>(type.get());
  }
  ir::BasicKind basicKind = rootType->getBasicKind();
  for (auto &[key, val] : values) {
    switch (basicKind) {
    case ir::BasicKind::I32:
      val = ir::Number(val.intValue());
      break;
    case ir::BasicKind::F32:
      val = ir::Number(val.floatValue());
      break;
    default:
      throw std::runtime_error("Unsupported type in makeGlobal");
    }
  }

  auto clonedType = type->clone();
  auto constant = fuseConst(std::move(clonedType), values, 0);

  auto symbol = std::make_unique<ir::GlobalVariable>(
      std::move(type), name, isConst, std::move(constant));
  _table.front()[name] = symbol.get();
  return std::move(symbol);
}

std::unique_ptr<ir::AllocaInst>
SymbolTable::makeLocal(std::unique_ptr<ir::Type> type,
                       const std::string &name) {
  auto symbol = std::make_unique<ir::AllocaInst>(std::move(type));
  _table.front()[name] = symbol.get();
  return std::move(symbol);
}

std::unique_ptr<ir::AllocaInst>
SymbolTable::makeLocal(std::unique_ptr<ir::Type> type, const std::string &name,
                       const std::vector<int> &dimensions) {
  for (int i = static_cast<int>(dimensions.size() - 1); i >= 0; i--)
    type = std::make_unique<ir::ArrayType>(std::move(type), dimensions[i]);

  auto allocaInst = std::make_unique<ir::AllocaInst>(std::move(type));
  _table.front()[name] = allocaInst.get();
  return std::move(allocaInst);
}

std::unique_ptr<ir::Argument>
SymbolTable::makeArgument(std::unique_ptr<ir::Type> type,
                          const std::string &name) {
  auto arg = std::make_unique<ir::Argument>(std::move(type), name);
  _table.front()[name] = arg.get();
  return std::move(arg);
}
