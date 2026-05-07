#pragma once

#include "IR.h"

extern int DEBUG;
namespace sysy {

// 优化工具类，包含一些通用的优化方法
// 这些方法可以在不同的优化 pass 中复用
// 例如：删除use关系,判断是否是全局变量等
class SysYIROptUtils{

public:
  struct PairHash {
      template <class T1, class T2>
      std::size_t operator () (const std::pair<T1, T2>& p) const {
          auto h1 = std::hash<T1>{}(p.first);
          auto h2 = std::hash<T2>{}(p.second);

          // 简单的组合哈希值，可以更复杂以减少冲突
          // 使用 boost::hash_combine 的简化版本
          return h1 ^ (h2 << 1); 
      }
  };

  static void RemoveUserOperandUses(User *user) {
    if (!user) {
        return;
    }

    // 遍历 User 的 operands 列表。
    // 由于 operands 是 protected 成员，我们需要一个临时方法来访问它，
    // 或者在 User 类中添加一个 friend 声明。
    // 假设 User 内部有一个像 getOperands() 这样的公共方法返回 operands 的引用，
    // 或者将 SysYIROptUtils 声明为 User 的 friend。
    // 为了示例，我将假设可以直接访问 user->operands 或通过一个getter。
    // 如果无法直接访问，请在 IR.h 的 User 类中添加：
    // public: const std::vector<std::shared_ptr<Use>>& getOperands() const { return operands; }

    // 迭代 copies of shared_ptr to avoid issues if removeUse modifies the list
    // (though remove should handle it, iterating a copy is safer or reverse iteration).
    // Since we'll clear the vector at the end, iterating forward is fine.
    for (const auto& use_ptr : user->getOperands()) { // 假设 getOperands() 可用
        if (use_ptr) {
            Value *val = use_ptr->getValue(); // 获取 Use 指向的 Value (如 AllocaInst)
            if (val) {
                val->removeUse(use_ptr); // 通知 Value 从其 uses 列表中移除此 Use 关系
            }
        }
    }
  }
  static void usedelete(Instruction *inst) {
    assert(inst && "Instruction to delete cannot be null.");
    BasicBlock *parentBlock = inst->getParent();
    assert(parentBlock && "Instruction must have a parent BasicBlock to be deleted.");

    // 步骤1: 处理所有使用者，将他们从使用 inst 变为使用 UndefinedValue
    // 这将清理 inst 作为 Value 时的 uses 列表
    if (!inst->getUses().empty()) {
        inst->replaceAllUsesWith(UndefinedValue::get(inst->getType()));
    }

    // 步骤2: 清理 inst 作为 User 时的操作数关系
    // 通知 inst 所使用的所有 Value (如 AllocaInst)，移除对应的 Use 关系。
    // 这里的 inst 实际上是一个 User*，所以可以安全地向下转型。
    RemoveUserOperandUses(static_cast<User*>(inst));

    // 步骤3: 物理删除指令
    // 这会导致 Instruction 对象的 unique_ptr 销毁，从而调用其析构函数链。
    parentBlock->removeInst(inst);
} 

  static BasicBlock::iterator usedelete(BasicBlock::iterator inst_it) {
    Instruction *inst_to_delete = inst_it->get();
    BasicBlock *parentBlock = inst_to_delete->getParent();
    assert(parentBlock && "Instruction must have a parent BasicBlock for iterator deletion.");

    // 步骤1: 处理所有使用者
    if (!inst_to_delete->getUses().empty()) {
        inst_to_delete->replaceAllUsesWith(UndefinedValue::get(inst_to_delete->getType()));
    }

    // 步骤2: 清理操作数关系
    RemoveUserOperandUses(static_cast<User*>(inst_to_delete));

    // 步骤3: 物理删除指令并返回下一个迭代器
    return parentBlock->removeInst(inst_it);
  }

  // 判断是否是全局变量
  static bool isGlobal(Value *val) {
    auto gval = dynamic_cast<GlobalValue *>(val);
    return gval != nullptr;
  }
  // 判断是否是数组
  static bool isArr(Value *val) {
    auto aval = dynamic_cast<AllocaInst *>(val);
    // 如果是 AllocaInst 且通过Type::isArray()判断为数组类型
    return aval && aval->getType()->as<PointerType>()->getBaseType()->isArray();
  }
  // 判断是否是指向数组的指针
  static bool isArrPointer(Value *val) {
    auto aval = dynamic_cast<AllocaInst *>(val);
    // 如果是 AllocaInst 且通过Type::isPointer()判断为指针;
    auto baseType = aval->getType()->as<PointerType>()->getBaseType();
    // 在sysy中，函数的数组参数会退化成指针
    // 所以当AllocaInst的basetype是PointerType时（一维数组）或者是指向ArrayType的PointerType（多位数组）时，返回true
    return aval && (baseType->isPointer() || baseType->as<PointerType>()->getBaseType()->isArray());
  }

  
  // PHI指令消除相关方法
  static bool eliminateRedundantPhisInFunction(Function* func){
    bool changed = false;
    std::vector<Instruction *> toDelete;
    for (auto &bb : func->getBasicBlocks()) {
      for (auto &inst : bb->getInstructions()) {
        if (auto phi = dynamic_cast<PhiInst *>(inst.get())) {
          auto incoming = phi->getIncomingValues();
          if(DEBUG){
            std::cout << "Checking Phi: " << phi->getName() << " with " << incoming.size() << " incoming values." << std::endl;
          }
          if (incoming.size() == 1) {
            Value *singleVal = incoming[0].second;
            inst->replaceAllUsesWith(singleVal);
            toDelete.push_back(inst.get());
          }
        }
        else
          break; // 只处理Phi指令
      }
    }
    for (auto *phi : toDelete) {
      usedelete(phi);
      changed = true; // 标记为已更改
    }
    return changed; // 返回是否有删除发生
  }

  //该实现参考了libdivide的算法
  static std::pair<int, int> computeMulhMagicNumbers(int divisor) {
    
    if (DEBUG) {
      std::cout << "\n[SR] ===== Computing magic numbers for divisor " << divisor << " (libdivide algorithm) =====" << std::endl;
    }
    
    if (divisor == 0) {
      if (DEBUG) std::cout << "[SR] Error: divisor must be != 0" << std::endl;
      return {-1, -1};
    }

    // libdivide 常数
    const uint8_t LIBDIVIDE_ADD_MARKER = 0x40;
    const uint8_t LIBDIVIDE_NEGATIVE_DIVISOR = 0x80;
    
    // 辅助函数：计算前导零个数
    auto count_leading_zeros32 = [](uint32_t val) -> uint32_t {
      if (val == 0) return 32;
      return __builtin_clz(val);
    };
    
    // 辅助函数：64位除法返回32位商和余数
    auto div_64_32 = [](uint32_t high, uint32_t low, uint32_t divisor, uint32_t* rem) -> uint32_t {
      uint64_t dividend = ((uint64_t)high << 32) | low;
      uint32_t quotient = dividend / divisor;
      *rem = dividend % divisor;
      return quotient;
    };

    if (DEBUG) {
      std::cout << "[SR] Input divisor: " << divisor << std::endl;
    }

    // libdivide_internal_s32_gen 算法实现
    int32_t d = divisor;
    uint32_t ud = (uint32_t)d;
    uint32_t absD = (d < 0) ? -ud : ud;
    
    if (DEBUG) {
      std::cout << "[SR] absD = " << absD << std::endl;
    }
    
    uint32_t floor_log_2_d = 31 - count_leading_zeros32(absD);
    
    if (DEBUG) {
      std::cout << "[SR] floor_log_2_d = " << floor_log_2_d << std::endl;
    }
    
    // 检查 absD 是否为2的幂
    if ((absD & (absD - 1)) == 0) {
      if (DEBUG) {
        std::cout << "[SR] " << absD << " 是2的幂，使用移位方法" << std::endl;
      }
      
      // 对于2的幂，我们只使用移位，不需要魔数
      int shift = floor_log_2_d;
      if (d < 0) shift |= 0x80; // 标记负数
      
      if (DEBUG) {
        std::cout << "[SR] Power of 2 result: magic=0, shift=" << shift << std::endl;
        std::cout << "[SR] ===== End magic computation =====" << std::endl;
      }
      
      // 对于我们的目的，我们将在IR生成中以不同方式处理2的幂
      // 返回特殊标记
      return {0, shift};
    }
    
    if (DEBUG) {
      std::cout << "[SR] " << absD << " is not a power of 2, computing magic number" << std::endl;
    }
    
    // 非2的幂除数的魔数计算
    uint8_t more;
    uint32_t rem, proposed_m;
    
    // 计算 proposed_m = floor(2^(floor_log_2_d + 31) / absD)
    proposed_m = div_64_32((uint32_t)1 << (floor_log_2_d - 1), 0, absD, &rem);
    const uint32_t e = absD - rem;
    
    if (DEBUG) {
      std::cout << "[SR] proposed_m = " << proposed_m << ", rem = " << rem << ", e = " << e << std::endl;
    }
    
    // 确定是否需要"加法"版本
    const bool branchfree = false; // 使用分支版本
    
    if (!branchfree && e < ((uint32_t)1 << floor_log_2_d)) {
      // 这个幂次有效
      more = (uint8_t)(floor_log_2_d - 1);
      if (DEBUG) {
        std::cout << "[SR] Using basic algorithm, shift = " << (int)more << std::endl;
      }
    } else {
      // 我们需要上升一个等级
      proposed_m += proposed_m;
      const uint32_t twice_rem = rem + rem;
      if (twice_rem >= absD || twice_rem < rem) {
        proposed_m += 1;
      }
      more = (uint8_t)(floor_log_2_d | LIBDIVIDE_ADD_MARKER);
      if (DEBUG) {
        std::cout << "[SR] Using add algorithm, proposed_m = " << proposed_m << ", more = " << (int)more << std::endl;
      }
    }
    
    proposed_m += 1;
    int32_t magic = (int32_t)proposed_m;
    
    // 处理负除数
    if (d < 0) {
      more |= LIBDIVIDE_NEGATIVE_DIVISOR;
      if (!branchfree) {
        magic = -magic;
      }
      if (DEBUG) {
        std::cout << "[SR] Negative divisor, magic = " << magic << ", more = " << (int)more << std::endl;
      }
    }
    
    // 为我们的IR生成提取移位量和标志  
    int shift = more & 0x3F; // 移除标志，保留移位量（位0-5）
    bool need_add = (more & LIBDIVIDE_ADD_MARKER) != 0;
    bool is_negative = (more & LIBDIVIDE_NEGATIVE_DIVISOR) != 0;
    
    if (DEBUG) {
      std::cout << "[SR] Final result: magic = " << magic << ", more = " << (int)more 
                << " (0x" << std::hex << (int)more << std::dec << ")" << std::endl;
      std::cout << "[SR] Shift = " << shift << ", need_add = " << need_add 
                << ", is_negative = " << is_negative << std::endl;
      
      // Test the magic number using the correct libdivide algorithm
      std::cout << "[SR] Testing magic number (libdivide algorithm):" << std::endl;
      int test_values[] = {1, 7, 37, 100, 999, -1, -7, -37, -100};
      
      for (int test_val : test_values) {
        int64_t quotient;
        
        // 实现正确的libdivide算法
        int64_t product = (int64_t)test_val * magic;
        int64_t high_bits = product >> 32;
        
        if (need_add) {
          // ADD_MARKER情况：移位前加上被除数
          // 这是libdivide的关键洞察！
          high_bits += test_val;
          quotient = high_bits >> shift;
        } else {
          // 正常情况：只是移位
          quotient = high_bits >> shift;
        }
        
        // 符号修正：这是libdivide有符号除法的关键部分！
        // 如果被除数为负，商需要加1来匹配C语言的截断除法语义
        if (test_val < 0) {
          quotient += 1;
        }
        
        int expected = test_val / divisor;
        
        bool correct = (quotient == expected);
        std::cout << "[SR]   " << test_val << " / " << divisor << " = " << quotient 
                  << " (expected " << expected << ") " << (correct ? "✓" : "✗") << std::endl;
      }
      
      std::cout << "[SR] ===== End magic computation =====" << std::endl;
    }
    
    // 返回魔数、移位量，并在移位中编码ADD_MARKER标志
    // 我们将使用移位的第6位表示ADD_MARKER，第7位表示负数（如果需要）
    int encoded_shift = shift;
    if (need_add) {
      encoded_shift |= 0x40; // 设置第6位表示ADD_MARKER
      if (DEBUG) {
        std::cout << "[SR] Encoding ADD_MARKER in shift: " << encoded_shift << std::endl;
      }
    }
    
    return {magic, encoded_shift};
  }

};

}// namespace sysy