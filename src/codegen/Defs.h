#pragma once

#include "codegen/Ops.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace codegen {

class Region;
class BasicBlock;
class Op;

enum class ValueType {
    Void,
    Int,
    Float,
};

class Value {
public:
    Value() = default;
    explicit Value(Op *from);

    Op *defining() const { return defining_; }

    bool operator==(Value rhs) const { return defining_ == rhs.defining_; }
    bool operator!=(Value rhs) const { return defining_ != rhs.defining_; }
    bool operator<(Value rhs) const { return defining_ < rhs.defining_; }
    bool operator>(Value rhs) const { return defining_ > rhs.defining_; }
    bool operator<=(Value rhs) const { return defining_ <= rhs.defining_; }
    bool operator>=(Value rhs) const { return defining_ >= rhs.defining_; }

private:
    Op *defining_ = nullptr;
};

class Attr {
public:
    Attr(std::string key, std::string value);

    const std::string &key() const { return key_; }
    const std::string &value() const { return value_; }

private:
    std::string key_;
    std::string value_;
};

class Op {
public:
    explicit Op(Opcode opcode, ValueType returnType = ValueType::Void,
                std::vector<Value> operands = {},
                std::optional<std::vector<Attr>> attrs = std::nullopt);

    Opcode opcode() const { return opcode_; }
    ValueType returnType() const { return returnType_; }
    const std::vector<Value> &operands() const { return operands_; }
    const std::optional<std::vector<Attr>> &attrs() const { return attrs_; }
    const std::unordered_set<Op *> &uses() const { return uses_; }

    BasicBlock *parent() const { return parent_; }
    bool eraseFromParent();

private:
    friend class BasicBlock;

    Opcode opcode_;
    ValueType returnType_ = ValueType::Void;
    std::vector<Value> operands_;
    std::optional<std::vector<Attr>> attrs_;
    std::unordered_set<Op *> uses_;
    BasicBlock *parent_ = nullptr;
};

class BasicBlock {
public:
    explicit BasicBlock(std::string name = {});

    const std::string &name() const { return name_; }
    Region *parent() const { return parent_; }

    std::size_t size() const { return ops_.size(); }
    bool empty() const { return ops_.empty(); }

    Op *appendOp(std::unique_ptr<Op> op);
    Op *insertOp(std::size_t index, std::unique_ptr<Op> op);
    bool eraseOp(Op *op);

    Op *opAt(std::size_t index) const;
    const std::vector<std::unique_ptr<Op>> &ops() const { return ops_; }

private:
    friend class Region;

    std::string name_;
    Region *parent_ = nullptr;
    std::vector<std::unique_ptr<Op>> ops_;
};

class Region {
public:
    explicit Region(std::string name = {});

    const std::string &name() const { return name_; }

    std::size_t size() const { return blocks_.size(); }
    bool empty() const { return blocks_.empty(); }

    BasicBlock *createBlock(std::string name = {});
    BasicBlock *insertBlock(std::size_t index, std::unique_ptr<BasicBlock> block);
    bool eraseBlock(BasicBlock *block);

    BasicBlock *blockAt(std::size_t index) const;
    const std::vector<std::unique_ptr<BasicBlock>> &blocks() const { return blocks_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
};

} // namespace codegen
