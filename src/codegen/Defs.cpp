#include "codegen/Defs.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace codegen {

Value::Value(Op *from) : defining_(from) {}

Attr::Attr(std::string key, std::string value) : key_(std::move(key)), value_(std::move(value)) {}

Op::Op(Opcode opcode, ValueType returnType, std::vector<Value> operands,
       std::optional<std::vector<Attr>> attrs)
    : opcode_(opcode), returnType_(returnType), operands_(std::move(operands)),
      attrs_(std::move(attrs)) {
    for (const Value operand : operands_) {
        if (operand.defining() != nullptr) {
            operand.defining()->uses_.insert(this);
        }
    }
}

bool Op::eraseFromParent() {
    return parent_ != nullptr && parent_->eraseOp(this);
}

BasicBlock::BasicBlock(std::string name) : name_(std::move(name)) {}

Op *BasicBlock::appendOp(std::unique_ptr<Op> op) {
    return insertOp(ops_.size(), std::move(op));
}

Op *BasicBlock::insertOp(std::size_t index, std::unique_ptr<Op> op) {
    if (!op) {
        throw std::invalid_argument("insertOp: op is null");
    }
    if (op->parent_ != nullptr) {
        throw std::invalid_argument("insertOp: op already has a parent block");
    }
    if (index > ops_.size()) {
        throw std::out_of_range("insertOp: index out of range");
    }
    op->parent_ = this;
    Op *raw = op.get();
    ops_.insert(ops_.begin() + static_cast<std::ptrdiff_t>(index), std::move(op));
    return raw;
}

bool BasicBlock::eraseOp(Op *op) {
    if (op == nullptr) {
        return false;
    }
    auto it = std::find_if(ops_.begin(), ops_.end(), [op](const std::unique_ptr<Op> &item) {
        return item.get() == op;
    });
    if (it == ops_.end()) {
        return false;
    }
    for (const Value operand : (*it)->operands_) {
        if (operand.defining() != nullptr) {
            operand.defining()->uses_.erase((*it).get());
        }
    }
    for (Op *use : (*it)->uses_) {
        for (Value &operand : use->operands_) {
            if (operand.defining() == (*it).get()) {
                operand = Value();
            }
        }
    }
    (*it)->parent_ = nullptr;
    ops_.erase(it);
    return true;
}

Op *BasicBlock::opAt(std::size_t index) const {
    if (index >= ops_.size()) {
        return nullptr;
    }
    return ops_[index].get();
}

Region::Region(std::string name) : name_(std::move(name)) {}

BasicBlock *Region::createBlock(std::string name) {
    auto block = std::make_unique<BasicBlock>(std::move(name));
    block->parent_ = this;
    BasicBlock *raw = block.get();
    blocks_.push_back(std::move(block));
    return raw;
}

BasicBlock *Region::insertBlock(std::size_t index, std::unique_ptr<BasicBlock> block) {
    if (!block) {
        throw std::invalid_argument("insertBlock: block is null");
    }
    if (block->parent_ != nullptr) {
        throw std::invalid_argument("insertBlock: block already has a parent region");
    }
    if (index > blocks_.size()) {
        throw std::out_of_range("insertBlock: index out of range");
    }
    block->parent_ = this;
    BasicBlock *raw = block.get();
    blocks_.insert(blocks_.begin() + static_cast<std::ptrdiff_t>(index), std::move(block));
    return raw;
}

bool Region::eraseBlock(BasicBlock *block) {
    if (block == nullptr) {
        return false;
    }
    auto it = std::find_if(blocks_.begin(), blocks_.end(),
                           [block](const std::unique_ptr<BasicBlock> &item) {
                               return item.get() == block;
                           });
    if (it == blocks_.end()) {
        return false;
    }
    (*it)->parent_ = nullptr;
    blocks_.erase(it);
    return true;
}

BasicBlock *Region::blockAt(std::size_t index) const {
    if (index >= blocks_.size()) {
        return nullptr;
    }
    return blocks_[index].get();
}

} // namespace codegen
