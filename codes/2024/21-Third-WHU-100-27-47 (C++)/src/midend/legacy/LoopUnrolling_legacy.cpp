// #include "LoopUnrolling.h"

// void ir::runLoopUnroll(ir::FuncPtr func) {
//     ir::LoopDetection loopFinder;
//     loopFinder.detectLoops(func);
//     for (auto loop : loopFinder.getLoops()) {
//         handleUnroll(loop);
//     }
//     return;
// }
// bool ir::getUnrollInfo(Ptr<ir::Loop> loop, ir::UnrollLoop &ret) {

//     // 循环头块
//     ret.header = loop->getHeaderBB();
//     // 循环基本块集合
//     ret.bbs = loop->getLoopBBs();
//     // 循环前驱块，用于设置参数等
//     for (auto pred : ret.header->preds())
//         if (std::find(ret.bbs.begin(), ret.bbs.end(), pred) == ret.bbs.end())
//             ret.preheader = pred;
//     // 循环体（不包括头块）
//     ret.bodies.assign(ret.bbs.begin() + 1, ret.bbs.end());
//     // 不对有子循环的循环进行展开
//     if (loop->getSubLoops().size() > 0) {
//         return false;
//     }
//     // 回边（指向头块）只有一条
//     if (loop->getBackEdgesBlocksSet().size() > 1) {
//         return false;
//     }

//     // that means we don't unroll a loop with break statement
//     if (loop->getExitBBMap().size() > 1) {
//         return false;
//     }

//     // 出口块为头块，循环不满足时从头块退出
//     if (loop->getExitBBMap().begin()->first != ret.header) {
//         return false;
//     }

//     // 循环出边所指基本块
//     ret.exit = loop->getExitBBMap().begin()->second;

//     // get the exitInst and the condition of the loop
//     auto exInst = ret.header->exitInst();
//     auto cond = exInst->condition();

//     // the condition should be a register
//     // if the condition is bool or something else, we don't have to unroll it
//     if (!cond.isRegister()) {
//         return false;
//     }

//     auto condReg = cond.getRegister();
//     auto cmpInst = castPtr<ir::OperInst>(condReg->defInst());
//     if (!cmpInst) {
//         return false;
//     }

//     ret.cond = cmpInst;
//     if (cmpInst->uses().size() != 2) {
//         return false;
//     }

//     auto lhs = cmpInst->lhs();
//     auto rhs = cmpInst->rhs();
//     auto op = cmpInst->op();

//     // more than one value in condition should be literal after constant folding
//     // do not unroll the loop looks like while(i<n)/*some code that change both i and n*/
//     if (lhs.isLiteral()) {
//         ret.ind_var = std::make_shared<ir::Value>(rhs);
//         ret.bound = std::make_shared<ir::Value>(lhs);
//     } else if (rhs.isLiteral()) {
//         ret.ind_var = std::make_shared<ir::Value>(lhs);
//         ret.bound = std::make_shared<ir::Value>(rhs);
//     } else {
//         return false;
//     }

//     // set initial and step
//     for (auto inst : ret.header->getInstSet()) {
//         if (std::dynamic_pointer_cast<ir::PhiInst>(inst) == nullptr)
//             break;
//         if (std::dynamic_pointer_cast<ir::Value>(inst) != ret.ind_var)
//             continue;
//         // the phi for induction variable
//         auto phiInst = std::dynamic_pointer_cast<ir::PhiInst>(inst);
//         // the phi should has two source, one from preheader and the other from loop body
//         dbgassert(phiInst->getTuples().size() == 2, "the phi should has two source");

//         for (ir::PhiTuple tup : phiInst->getTuples()) {
//             if (std::find(loop->getLoopBBs().begin(), loop->getLoopBBs().end(), tup.bb()) != loop->getLoopBBs().end()) {
//                 if (!tup.value().isRegister()) {
//                     continue;
//                 }
//                 // 获取定义当前变量的指令
//                 // 即找到修改步长i的指令 ------> while(i < 10) /* code */
//                 auto assignIndVar = tup.value().getRegister()->defInst();
//                 if (std::dynamic_pointer_cast<ir::OperInst>(assignIndVar) == nullptr) {
//                     continue;
//                 }
//                 auto operInst = std::dynamic_pointer_cast<ir::OperInst>(assignIndVar);
//                 auto OperOp = operInst->op();
//                 // 只对通过加减进行增减的步长进行展开优化
//                 if (OperOp != ir::OperInst::Operator::Add && OperOp != ir::OperInst::Operator::Sub) {
//                     continue;
//                 }
//                 if (operInst->lhs().isLiteral() && std::make_shared<ir::Value>(operInst->rhs()) == ret.ind_var) {
//                     ret.step = std::make_shared<ir::Value>(operInst->lhs());
//                 } else if (operInst->rhs().isLiteral() && std::make_shared<ir::Value>(operInst->lhs()) == ret.ind_var) {
//                     ret.step = std::make_shared<ir::Value>(operInst->rhs());
//                 }
//             } else {
//                 // initial
//                 if (!tup.value().isLiteral()) {
//                     continue;
//                 }
//                 ret.initial = std::make_shared<ir::Value>(tup.value());
//             }
//         }
//     }

//     if (ret.initial == nullptr or ret.step == nullptr) {
//         return false;
//     }

//     // return ret;
//     return true;
// }

// /*
// while(i < n){
//     i = i + step;
// }
// */
// bool ir::doUnroll(ir::UnrollLoop loop) {
//     auto start = loop.initial->getLiteral();
//     auto end = loop.bound->getLiteral();
//     auto step = loop.step->getLiteral();

//     if ((start.isInt() || start.isFloat()) && (end.isInt() || end.isFloat()) && (step.isInt() || step.isFloat())) {
//         int t = (end.getInt() - start.getInt()) / step.getInt();
//         // float t = (end.getFloat() - start.getFloat()) / step.getFloat();
//         return t >= ir::UNROLLT;
//     }

//     return false;
// }
// /*
// 展开前：
// entry:
//     br label %loop

// loop:                                              ; preds = %entry, %loop
//     %i = phi i32 [ 0, %entry ], [ %next_i, %loop ]  ; Phi 指令
//     call void @printf(i32 %i)
//     %next_i = add i32 %i, 1                        ; 计算下一个迭代的值
//     %loop_cond = icmp slt i32 %next_i, 5           ; 比较操作
//     br i1 %loop_cond, label %loop, label %exit     ; 根据比较结果跳转

// exit:
//     ret void

// 展开后：
// entry:
//     br label %loop

// loop:                                              ; preds = %entry, %loop
//     %i = phi i32 [ 0, %entry ], [ %next_i_2, %loop ]  ; 更新的 Phi 指令
//     call void @printf(i32 %i)
//     %next_i_1 = add i32 %i, 1                       ; 第一次展开的更新值
//     call void @printf(i32 %next_i_1)
//     %next_i_2 = add i32 %next_i_1, 1                ; 第二次展开的更新值
//     %loop_cond = icmp slt i32 %next_i_2, 5          ; 比较操作
//     br i1 %loop_cond, label %loop, label %exit      ; 根据比较结果跳转

// exit:
//     ret void

// PhiMap函数用于确保每次展开迭代中 Phi 指令正确引用前一迭代的更新值
// */
// ir::Value PhiMap(ir::InstPtr inst, Map<ir::InstPtr, Vector<ir::InstPtr>> instMap, int times, const ir::UnrollLoop &loop) {
//     dbgassert(std::dynamic_pointer_cast<ir::PhiInst>(inst) != nullptr, "Not a Phi Instruction");
//     if (auto phiInst = std::dynamic_pointer_cast<ir::PhiInst>(inst)) {
//         auto tups = phiInst->getTuples();
//         for (auto tup : tups) {
//             auto val = tup.value();
//             auto src = tup.bb();
//             if (std::find(loop.bbs.begin(), loop.bbs.end(), src) != loop.bbs.end()) {
//                 // 若源值来自指令
//                 if (auto nextPhiInst = std::dynamic_pointer_cast<ir::PhiInst>(val.getRegister()->defInst())) {
//                     // 父基本块为循环头基本块，则需要递归向第一次循环的方向查找变量更新信息
//                     if (nextPhiInst->parentBB() == loop.header && times > 1) {
//                         return PhiMap(nextPhiInst, instMap, times - 1, loop);
//                     } else {
//                         return *instMap[nextPhiInst][times - 1]->defs().begin();
//                     }
//                 } else {
//                     return val;
//                 }
//             }
//         }
//     }
//     // 返回空Value
//     return ir::Value();
// }

// // 循环展开的具体操作函数
// void ir::unrollFunction(ir::UnrollLoop loop) {
//     // 生成新的头基本块、循环体，并从原循环体拷贝指令
//     auto func = loop.header->parentFunc();
//     ir::BBPtr newHeader = ir::BasicBlock::create(func, "_new");
//     ir::BBPtr bodyEntry;
//     Map<ir::BBPtr, Vector<ir::BBPtr>> bbMap;
//     Map<ir::InstPtr, Vector<ir::InstPtr>> instMap;
//     // 加入映射关系中
//     bbMap[loop.header].push_back(newHeader);
//     // 复制循环头块中指令
//     for (auto inst : loop.header->getInstTopoList()) {
//         ir::InstPtr newInst;
//         // TODO
//         newInst = ir::Instruction::clone(inst, newHeader);
//         instMap[inst].push_back(newInst);
//     }
//     // 为新的循环体增加基本块
//     Vector<ir::BBPtr> newBBs;
//     for (int i = 0; i < UNROLLT; ++i) {
//         for (auto bb : loop.bodies) {
//             auto newBB = ir::BasicBlock::create(func, "_new");
//             newBBs.push_back(newBB);
//             bbMap[bb].push_back(newBB);
//             for (auto inst : bb->getInstTopoList()) {
//                 ir::InstPtr newInst;
//                 newInst = ir::Instruction::clone(inst, newHeader);
//                 instMap[inst].push_back(newInst);
//             }
//         }
//     }
//     // 对新头基本块中的指令进行操作数修改
//     for (auto newInst : newHeader->getInstTopoList()) {
//         for (auto operand : newInst->useRefs()) {
//             // 若操作数为（由inst指令定义的）寄存器
//             if (ir::InstPtr inst = std::dynamic_pointer_cast<ir::Instruction>(operand.get()->defInst())) {
//                 auto parentBB = inst->parentBB();
//                 // 若该操作数的定义指令属于头基本块且为Phi指令
//                 if (auto phi = std::dynamic_pointer_cast<ir::PhiInst>(newInst)) {
//                     if (parentBB == newHeader) {
//                         // 找到当前operands对应的BasicBlock以便进行updateTuple操作
//                         for (auto i : phi->getTuples()) {
//                             if (i.value().isRegister() && operand.get() == i.value().getRegister()) {
//                                 phi->updateTuple(i.bb(), PhiMap(inst, instMap, ir::UNROLLT - 1, loop));
//                             }
//                         }
//                     }
//                 } else if (std::find(loop.bbs.begin(), loop.bbs.end(), parentBB) != loop.bbs.end()) {
//                     for (auto def : instMap[inst].back()->defs()) {
//                         operand.replace(def);
//                     }
//                 }
//             }

//             if (loop.bound->getLiteral() == operand.get()->constVal().getLiteral() &&
//                 newInst == instMap[loop.cond].front()) {
//                 int initial = loop.initial->getLiteral().getInt();
//                 int step = loop.step->getLiteral().getInt();
//                 int bound = loop.bound->getLiteral().getInt();
//                 int newBoundT = bound - (bound - initial) % step * UNROLLT;
//                 auto newBound = ir::Value(ir::Literal(newBoundT));
//                 operand.replace(newBound);
//             }
//         }
//         // 若操作数为跳转指令
//         if (auto exitInst = castPtr<ir::ExitInst>(newInst)) {
//             // 出口处理
//             auto bbTrue = exitInst->trueTarget();
//             auto bbFalse = exitInst->falseTarget();
//             // 对真值跳转出口进行调整
//             if (std::find(loop.bbs.begin(), loop.bbs.end(), bbTrue) != loop.bbs.end()) {
//                 bodyEntry = bbTrue;
//                 // 修改出口
//                 exitInst->condBrTrueEdge()->redirectDest(bbMap[bbTrue].front());
//             } else if (bbTrue == loop.exit) {
//                 exitInst->condBrTrueEdge()->redirectDest(loop.header);
//             }
//             if (std::find(loop.bbs.begin(), loop.bbs.end(), bbFalse) != loop.bbs.end()) {
//                 bodyEntry = bbFalse;
//                 // 修改出口
//                 exitInst->condBrFalseEdge()->redirectDest(bbMap[bbFalse].front());
//             } else if (bbFalse == loop.exit) {
//                 exitInst->condBrFalseEdge()->redirectDest(loop.header);
//             }
//         } else if (auto PhiInst = castPtr<ir::PhiInst>(newInst)) {
//             for (auto i : PhiInst->getTuples()) {
//                 auto newBB = bbMap[i.bb()].front();
//                 // TODO : 根据i.value()查找到对应tuple，并更新newBB
//                 // PhiInst->updateTuple(newBB, i.value());
//             }
//         }
//     }

//     // 对新的body块进行操作数更改
//     for (int i = 0; i < UNROLLT; ++i) {
//         for (auto bb : loop.bbs) {
//             auto newBB = bbMap[bb].at(i);
//             // 处理新块中的指令
//             for (auto inst : newBB->getInstTopoList()) {
//                 for (auto operand : inst->useRefs()) {
//                     // 若操作数为（由inst指令定义的）寄存器
//                     if (ir::InstPtr inst = std::dynamic_pointer_cast<ir::Instruction>(operand.get()->defInst())) {
//                         auto parentBB = inst->parentBB();
//                         // 若该操作数的定义指令属于头基本块且为Phi指令
//                         if (auto phi = std::dynamic_pointer_cast<ir::PhiInst>(inst)) {
//                             if (parentBB == newHeader) {
//                                 // 找到当前operands对应的BasicBlock以便进行updateTuple操作
//                                 for (auto i : phi->getTuples()) {
//                                     if (i.value().isRegister() && operand.get() == i.value().getRegister()) {
//                                         phi->updateTuple(i.bb(), PhiMap(inst, instMap, ir::UNROLLT - 1, loop));
//                                     }
//                                 }
//                             }
//                         } else if (std::find(loop.bbs.begin(), loop.bbs.end(), parentBB) != loop.bbs.end()) {
//                             for (auto def : instMap[inst].back()->defs()) {
//                                 operand.replace(def);
//                             }
//                         }
//                     }

//                     if (loop.bound->getLiteral() == operand.get()->constVal().getLiteral() &&
//                         inst == instMap[loop.cond].front()) {
//                         int initial = loop.initial->getLiteral().getInt();
//                         int step = loop.step->getLiteral().getInt();
//                         int bound = loop.bound->getLiteral().getInt();
//                         int newBoundT = bound - (bound - initial) % step * UNROLLT;
//                         auto newBound = ir::Value(ir::Literal(newBoundT));
//                         operand.replace(newBound);
//                     }
//                 }
//                 // 若操作数为跳转指令
//                 if (auto exitInst = castPtr<ir::ExitInst>(inst)) {
//                     // TODO 无条件跳转怎么办？
//                     // 出口处理
//                     auto bbTrue = exitInst->trueTarget();
//                     auto bbFalse = exitInst->falseTarget();
//                     // 对真值跳转出口进行调整
//                     if (std::find(loop.bbs.begin(), loop.bbs.end(), bbTrue) != loop.bbs.end()) {
//                         bodyEntry = bbTrue;
//                         // 修改出口
//                         exitInst->condBrTrueEdge()->redirectDest(bbMap[bbTrue].front());
//                     } else if (bbTrue == loop.exit) {
//                         exitInst->condBrTrueEdge()->redirectDest(loop.header);
//                     }
//                     if (std::find(loop.bbs.begin(), loop.bbs.end(), bbFalse) != loop.bbs.end()) {
//                         bodyEntry = bbFalse;
//                         // 修改出口
//                         exitInst->condBrFalseEdge()->redirectDest(bbMap[bbFalse].front());
//                     } else if (bbFalse == loop.exit) {
//                         exitInst->condBrFalseEdge()->redirectDest(loop.header);
//                     }
//                 } else if (auto PhiInst = castPtr<ir::PhiInst>(inst)) {
//                     for (auto i : PhiInst->getTuples()) {
//                         auto newBB = bbMap[i.bb()].front();
//                         // 根据i.value()查找到对应tuple，并更新newBB
//                         // PhiInst->updateTuple(PhiInst, newBB, i.value());
//                     }
//                 }
//             }
//         }
//     }

//     // 对原有块进行操作数更改 - Branch
//     for (auto inst : loop.preheader->getInstTopoList()) {
//         if (auto exitInst = castPtr<ir::ExitInst>(inst)) {
//             // 无条件跳转
//             if (exitInst->isUncondBr() && exitInst->unconditionalTarget() == newHeader) {
//                 exitInst->uncondBrEdge()->redirectDest(newHeader);
//             } else if (exitInst->isCondBr()) {
//                 if (exitInst->falseTarget() == newHeader) {
//                     exitInst->condBrFalseEdge()->redirectDest(newHeader);
//                 } else if (exitInst->trueTarget() == newHeader) {
//                     exitInst->condBrTrueEdge()->redirectDest(newHeader);
//                 }
//             }
//         }
//     }

//     // 对原有块进行操作数更改 - Phi
//     for (auto inst : loop.header->getInstTopoList()) {
//         if (auto PhiInst = castPtr<ir::PhiInst>(inst)) {
//             auto newReg = *instMap[inst].back()->defs().begin();
//             // 删除原先Phi函数来自loop.preheader的边
//             // 新增一对Tuple : (newReg, newHeader)
//             // PhiInst->updateTuple(PhiInst, )
//         }
//     }

//     // 基本块的连接边处理
//     loop.preheader->removeEdge(loop.preheader, loop.header);
//     if (std::find(loop.preheader->succs().begin(), loop.preheader->succs().end(), newHeader) == loop.preheader->succs().end()) {
//         loop.preheader->addEdge(loop.preheader, newHeader);
//     }

//     // 4.2 modify the pre_bb and succ_bb of new_header
//     // 修改新增头块的前驱后继基本块信息
//     // for (auto &new_inst : new_header->get_instructions()) {
//     //     if ((&new_inst)->is_phi())
//     //         for (unsigned i = 1; i < (&new_inst)->get_num_operand(); i += 2) {
//     //             auto bb =
//     //                 dynamic_cast<BasicBlock *>((&new_inst)->get_operand(i));
//     //             if (std::find(new_header->get_pre_basic_blocks().begin(),
//     //                           new_header->get_pre_basic_blocks().end(),
//     //                           bb) == new_header->get_pre_basic_blocks().end())
//     //                 new_header->add_pre_basic_block(bb);
//     //         }
//     //     else
//     //         break;
//     // }

//     for (auto newInst : newHeader->getInstTopoList()) {
//         if (auto phiInst = castPtr<ir::PhiInst>(newInst)) {
//             // 若newHeader中的Phi指令中有来自于bb的边
//             // 但newHeader的前驱块中没有bb，则增加
//         }
//     }

//     if (auto exitInst = castPtr<ir::ExitInst>(newHeader->getInstTopoList().back())) {
//         for (auto succ : newHeader->succs()) {
//             // 如果跳转指令的目的地不是newHeader的succs，则删除两者之间的边
//             bool flag = false;
//             if (exitInst->isBranch()) {
//                 if (exitInst->isUncondBr() && succ == exitInst->unconditionalTarget()) {
//                     flag = true;
//                 } else if (exitInst->isCondBr() && (succ == exitInst->falseTarget() || succ == exitInst->trueTarget())) {
//                     flag = true;
//                 }
//             }
//             if (!flag) {
//                 newHeader->removeEdge(newHeader, succ);
//                 succ->removeEdge(newHeader, succ);
//             }
//         }
//         if (exitInst->isCondBr()) {
//             auto bbTrue = exitInst->trueTarget();
//             if (std::find(newHeader->succs().begin(), newHeader->succs().end(), bbTrue) == newHeader->succs().end()) {
//                 newHeader->addEdge(newHeader, bbTrue);
//             }
//             if (std::find(newHeader->preds().begin(), newHeader->preds().end(), bbTrue) == newHeader->preds().end()) {
//                 newHeader->addEdge(bbTrue, newHeader);
//             }

//             auto bbFalse = exitInst->falseTarget();
//             if (std::find(newHeader->succs().begin(), newHeader->succs().end(), bbFalse) == newHeader->succs().end()) {
//                 newHeader->addEdge(newHeader, bbFalse);
//             }
//             if (std::find(newHeader->preds().begin(), newHeader->preds().end(), bbFalse) == newHeader->preds().end()) {
//                 newHeader->addEdge(bbFalse, newHeader);
//             }
//         } else if (exitInst->isUncondBr()) {
//             auto bb = exitInst->unconditionalTarget();
//             if (std::find(newHeader->succs().begin(), newHeader->succs().end(), bb) == newHeader->succs().end()) {
//                 newHeader->addEdge(newHeader, bb);
//             }
//             if (std::find(newHeader->preds().begin(), newHeader->preds().end(), bb) == newHeader->preds().end()) {
//                 newHeader->addEdge(bb, newHeader);
//             }
//         }
//     }

//     // 4.3 修改新增基本块的前驱后继基本块信息

//     for (int i = 0; i < UNROLLT; ++i) {
//         for (auto bb : loop.bodies) {
//             auto newBB = bbMap[bb].at(i);
//             if (auto exitInst = castPtr<ir::ExitInst>(newBB->getInstTopoList().back())) {
//                 for (auto succ : newBB->succs()) {
//                     // 如果跳转指令的目的地不是newBB的succs，则删除两者之间的边
//                     bool flag = false;
//                     if (exitInst->isBranch()) {
//                         if (exitInst->isUncondBr() && succ == exitInst->unconditionalTarget()) {
//                             flag = true;
//                         } else if (exitInst->isCondBr() && (succ == exitInst->falseTarget() || succ == exitInst->trueTarget())) {
//                             flag = true;
//                         }
//                     }
//                     if (!flag) {
//                         newBB->removeEdge(newBB, succ);
//                         succ->removeEdge(newBB, succ);
//                     }
//                 }

//                 if (exitInst->isCondBr()) {
//                     auto bbTrue = exitInst->trueTarget();
//                     if (std::find(newBB->succs().begin(), newBB->succs().end(), bbTrue) == newBB->succs().end()) {
//                         newBB->addEdge(newBB, bbTrue);
//                     }
//                     if (std::find(newBB->preds().begin(), newBB->preds().end(), bbTrue) == newBB->preds().end()) {
//                         newBB->addEdge(bbTrue, newBB);
//                     }

//                     auto bbFalse = exitInst->falseTarget();
//                     if (std::find(newBB->succs().begin(), newBB->succs().end(), bbFalse) == newBB->succs().end()) {
//                         newBB->addEdge(newBB, bbFalse);
//                     }
//                     if (std::find(newBB->preds().begin(), newBB->preds().end(), bbFalse) == newBB->preds().end()) {
//                         newBB->addEdge(bbFalse, newBB);
//                     }
//                 } else if (exitInst->isUncondBr()) {
//                     auto bb = exitInst->unconditionalTarget();
//                     if (std::find(newBB->succs().begin(), newBB->succs().end(), bb) == newBB->succs().end()) {
//                         newBB->addEdge(newBB, bb);
//                     }
//                     if (std::find(newBB->preds().begin(), newBB->preds().end(), bb) == newBB->preds().end()) {
//                         newBB->addEdge(bb, newBB);
//                     }
//                 }
//             }
//         }
//     }

//     loop.header->removeEdge(loop.preheader, loop.header);
// }
// void ir::handleUnroll(Ptr<ir::Loop> loop) {
//     ir::UnrollLoop ret;
//     // 未解析Loop信息则返回，说明当前Loop不符合优化标准
//     if (!getUnrollInfo(loop, ret)) {
//         return;
//     }
//     // 计算循环块运行次数，大于6才进行优化
//     if (doUnroll(ret)) {
//         unrollFunction(ret);
//     }
//     return;
// }
// namespace ir {
//     // test loop detection function
//     void testLoops1(ir::FuncPtr func) {
//         // 初始化LoopDetection并检测循环
//         ir::LoopDetection test;
//         test.detectLoops(func);
//         Vector<Ptr<Loop>> ans = test.getLoops();

//         if (ans.empty()) {
//             dbgout << "Could Not Get Loops" << std::endl;
//             return;
//         }

//         dbgout << "Loop Getting Done" << std::endl;
//         dbgout << "Loop Number: " << ans.size() << std::endl;

//         // 测试Loop类方法
//         for (auto loop : ans) {
//             // 测试getHeaderBB
//             ir::BBPtr header = loop->getHeaderBB();
//             dbgout << header->toString() << std::endl;
//             dbgassert(header != nullptr, "HeaderBB should not be null.");

//             // 测试addBlock和getLoopBBs
//             size_t initialSize = loop->getLoopBBs().size();
//             loop->addBlock(header);
//             dbgassert(loop->getLoopBBs().size() == initialSize + 1, "LoopBBs size should increase by 1.");

//             // 测试getPreheaderBB和setPreheaderBB
//             dbgassert(loop->getPreheaderBB() == nullptr, "PreheaderBB should be null initially.");
//             loop->setPreheaderBB(header);
//             dbgout << loop->getPreheaderBB()->toString() << std::endl;
//             dbgassert(loop->getPreheaderBB() == header, "PreheaderBB should be set correctly.");

//             // 测试getParentLoop和setParentLoop
//             dbgassert(loop->getParentLoop() == nullptr, "ParentLoop should be null initially.");
//             loop->setParentLoop(loop);
//             dbgassert(loop->getParentLoop() == loop, "ParentLoop should be set correctly.");

//             // 测试addSubLoop和getSubLoops
//             size_t subLoopsSize = loop->getSubLoops().size();
//             loop->addSubLoop(loop);
//             dbgassert(loop->getSubLoops().size() == subLoopsSize + 1, "SubLoops size should increase by 1.");

//             // 测试getBackEdgesBlocksSet和addBackEdgesBlocks
//             size_t backEdgesSize = loop->getBackEdgesBlocksSet().size();
//             loop->addBackEdgesBlocks(header);
//             dbgassert(loop->getBackEdgesBlocksSet().size() == backEdgesSize + 1, "BackEdgesBlocksSet size should increase by 1.");

//             // 测试getExitBBMap和addExitBB
//             size_t exitBBMapSize = loop->getExitBBMap().size();
//             // BasicBlock::create(func,"test");
//             // loop->addExitBB(header, BasicBlock::create(func, "test"));
//             // dbgassert(loop->getExitBBMap().size() == exitBBMapSize + 1, "ExitBBMap size should increase by 1.");
//             // dbgassert(loop->getExitBBMap().find(header) != loop->getExitBBMap().end(), "ExitBBMap should have the correct mapping.");

//             // 测试isLoopSimplifyForm
//             dbgassert(loop->isLoopSimplifyForm() == (loop->getPreheaderBB() != nullptr && loop->getBackEdgesBlocksSet().size() == 1),
//                       "Loop should satisfy Loop Simplify Form conditions.");
//         }
//     }
// }
