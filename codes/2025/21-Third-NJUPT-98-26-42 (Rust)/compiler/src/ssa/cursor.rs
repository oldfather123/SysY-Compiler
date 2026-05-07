//! Cursor -- 用于插入和操作DFG
//!
//! 警告：尽量不要绕过Layout/Cursor直接操作DFG和Layout
//! 通过 FuncCursor 可以方便地在函数的任意位置插入、删除或替换指令。

use crate::prelude::*;
use crate::ssa::{ctx::CoreContext, dfg::DataFlowGraph, function::Function, inst_builder::*};

use super::layout::*;

/// 游标的可能位置
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CursorPosition {
    /// 游标不指向任何位置，不能插入指令
    Nowhere,
    /// 游标指向一个现有指令
    /// 新指令将被插入在当前指令*之前*
    At(Inst),
    /// 游标在块的开头之前
    /// 不能插入指令，调用 next_inst() 将移动到块中的第一条指令
    Before(Block),
    /// 游标指向块的末尾之后
    /// 新指令将被追加到块的末尾
    After(Block),
}

/// 所有游标类型都实现的 Cursor trait，提供通用的导航操作
pub trait Cursor {
    /// 获取当前游标位置
    fn position(&self) -> CursorPosition;

    /// 设置当前位置
    fn set_position(&mut self, pos: CursorPosition);

    /// 获取Layout的引用
    fn layout(&self) -> &Layout;

    /// 获取Layout的可变引用
    fn layout_mut(&mut self) -> &mut Layout;

    /// 在特定位置重建游标
    fn at_position(mut self, pos: CursorPosition) -> Self
    where
        Self: Sized,
    {
        self.set_position(pos);
        self
    }

    /// 在指令处重建游标
    fn at_inst(mut self, inst: Inst) -> Self
    where
        Self: Sized,
    {
        self.goto_inst(inst);
        self
    }

    /// 在块的第一个插入点重建游标
    fn at_first_insertion_point(mut self, block: Block) -> Self
    where
        Self: Sized,
    {
        self.goto_first_insertion_point(block);
        self
    }

    /// 在块的第一条指令处重建游标
    fn at_first_inst(mut self, block: Block) -> Self
    where
        Self: Sized,
    {
        self.goto_first_inst(block);
        self
    }

    /// 在块的最后一条指令处重建游标
    fn at_last_inst(mut self, block: Block) -> Self
    where
        Self: Sized,
    {
        self.goto_last_inst(block);
        self
    }

    /// 在指令之后重建游标
    fn after_inst(mut self, inst: Inst) -> Self
    where
        Self: Sized,
    {
        self.goto_after_inst(inst);
        self
    }

    /// 在块的顶部重建游标
    fn at_top(mut self, block: Block) -> Self
    where
        Self: Sized,
    {
        self.goto_top(block);
        self
    }

    /// 在块的底部重建游标
    fn at_bottom(mut self, block: Block) -> Self
    where
        Self: Sized,
    {
        self.goto_bottom(block);
        self
    }

    /// 获取当前位置对应的块（如果有的话）
    fn current_block(&self) -> Option<Block> {
        use CursorPosition::*;
        match self.position() {
            Nowhere => None,
            At(inst) => self.layout().inst_block(inst),
            Before(block) | After(block) => Some(block),
        }
    }

    /// 获取当前位置对应的指令（如果有的话）
    fn current_inst(&self) -> Option<Inst> {
        use CursorPosition::*;
        match self.position() {
            At(inst) => Some(inst),
            _ => None,
        }
    }

    /// 移动到特定指令之后的位置
    /// 新指令将被插入在 inst 之后
    fn goto_after_inst(&mut self, inst: Inst) {
        debug_assert!(self.layout().inst_block(inst).is_some());

        let new_pos = if let Some(next) = self.layout().next_inst(inst) {
            CursorPosition::At(next)
        } else {
            CursorPosition::After(
                self.layout()
                    .inst_block(inst)
                    .expect("current instruction removed?"),
            )
        };
        self.set_position(new_pos);
    }

    /// 移动到特定指令的位置
    /// 新指令将被插入在 inst 之前
    fn goto_inst(&mut self, inst: Inst) {
        debug_assert!(self.layout().inst_block(inst).is_some());
        self.set_position(CursorPosition::At(inst));
    }

    /// 移动到块的第一个插入点
    fn goto_first_insertion_point(&mut self, block: Block) {
        if let Some(inst) = self.layout().first_inst(block) {
            self.goto_inst(inst);
        } else {
            self.goto_bottom(block);
        }
    }

    /// 移动到块的第一条指令
    fn goto_first_inst(&mut self, block: Block) {
        let inst = self.layout().first_inst(block).expect("空块");
        self.goto_inst(inst);
    }

    /// 移动到块的最后一条指令
    fn goto_last_inst(&mut self, block: Block) {
        let inst = self.layout().last_inst(block).expect("空块");
        self.goto_inst(inst);
    }

    /// 移动到块的顶部
    fn goto_top(&mut self, block: Block) {
        self.set_position(CursorPosition::Before(block));
    }

    /// 移动到块的底部
    fn goto_bottom(&mut self, block: Block) {
        self.set_position(CursorPosition::After(block));
    }

    /// 移动到布局中的下一个块并返回它
    ///
    /// - 如果游标没有指向任何位置，移动到第一个块
    /// - 如果没有更多块，游标指向 Nowhere 并返回 None
    fn next_block(&mut self) -> Option<Block> {
        let next = if let Some(block) = self.current_block() {
            self.layout().next_block(block)
        } else {
            // 尝试找到入口块
            self.layout().entry_block()
        };

        self.set_position(match next {
            Some(block) => CursorPosition::Before(block),
            None => CursorPosition::Nowhere,
        });
        next
    }

    /// 移动到布局中的上一个块并返回它
    ///
    /// - 如果游标没有指向任何位置，移动到最后一个块
    /// - 如果没有更多块，游标指向 Nowhere 并返回 None
    fn prev_block(&mut self) -> Option<Block> {
        let prev = if let Some(block) = self.current_block() {
            self.layout().prev_block(block)
        } else {
            // 尝试找到最后一个块
            self.layout().last_block()
        };

        self.set_position(match prev {
            Some(block) => CursorPosition::After(block),
            None => CursorPosition::Nowhere,
        });
        prev
    }

    /// 移动到同一块中的下一条指令并返回它
    fn next_inst(&mut self) -> Option<Inst> {
        use CursorPosition::*;

        match self.position() {
            Nowhere => None,
            Before(block) => {
                // 移动到块的第一条指令
                if let Some(inst) = self.layout().first_inst(block) {
                    self.set_position(At(inst));
                    Some(inst)
                } else {
                    self.set_position(After(block));
                    None
                }
            }
            At(inst) => {
                // 找到当前指令的下一条
                let block = self.layout().inst_block(inst)?;
                if let Some(next) = self.layout().next_inst(inst) {
                    self.set_position(At(next));
                    Some(next)
                } else {
                    self.set_position(After(block));
                    None
                }
            }
            After(_) => None,
        }
    }

    /// 移动到同一块中的上一条指令并返回它
    fn prev_inst(&mut self) -> Option<Inst> {
        use CursorPosition::*;

        match self.position() {
            Nowhere | Before(_) => None,
            After(block) => {
                // 移动到块的最后一条指令
                if let Some(inst) = self.layout().last_inst(block) {
                    self.set_position(At(inst));
                    Some(inst)
                } else {
                    self.set_position(Before(block));
                    None
                }
            }
            At(inst) => {
                // 找到当前指令的上一条
                let block = self.layout().inst_block(inst)?;
                if let Some(prev) = self.layout().prev_inst(inst) {
                    self.set_position(At(prev));
                    Some(prev)
                } else {
                    self.set_position(Before(block));
                    None
                }
            }
        }
    }

    /// 在当前位置插入指令
    fn insert_inst(&mut self, inst: Inst) {
        use self::CursorPosition::*;
        match self.position() {
            Nowhere | Before(..) => panic!("Invalid insert_inst position"),
            At(cur) => self.layout_mut().insert_inst(inst, cur),
            After(block) => self.layout_mut().append_inst(inst, block),
        }
    }

    /// 移除当前位置的指令，并移动到下一条指令
    fn remove_inst(&mut self) -> Inst {
        let inst = self.current_inst().expect("No instruction to remove");
        self.next_inst();
        self.layout_mut().remove_inst(inst);
        inst
    }

    /// 移除当前位置的指令，并移动到上一条指令
    fn remove_inst_and_step_back(&mut self) -> Inst {
        let inst = self.current_inst().expect("No instruction to remove");
        self.prev_inst();
        self.layout_mut().remove_inst(inst);
        inst
    }

    /// 替换当前位置的指令
    fn replace_inst(&mut self, new_inst: Inst) -> Inst {
        let prev_inst = self.remove_inst();
        self.insert_inst(new_inst);
        prev_inst
    }
    /// 当前位置插入一个新块，并移入
    fn insert_block(&mut self, new_block: Block) {
        use self::CursorPosition::*;
        match self.position() {
            At(inst) => {
                self.layout_mut().split_block(new_block, inst);
                return;
            }
            Nowhere => self.layout_mut().append_block(new_block),
            Before(block) => self.layout_mut().insert_block(new_block, block),
            After(block) => self.layout_mut().insert_block_after(new_block, block),
        }
        self.set_position(After(new_block));
    }
}

/// 函数游标
///
/// FuncCursor 持有对整个函数和核心上下文的可变引用，同时保持位置信息。
/// 这个游标用于在合法化之前操作指令。
pub struct FuncCursor<'a> {
    pos: CursorPosition,
    /// 引用的函数
    pub func: &'a mut Function,
    /// 核心上下文 -- 用于类型推导
    pub ctx: &'a mut CoreContext,
}

impl<'a> FuncCursor<'a> {
    /// 创建一个新的 FuncCursor，初始不指向任何位置
    pub fn new(func: &'a mut Function, ctx: &'a mut CoreContext) -> Self {
        Self {
            pos: CursorPosition::Nowhere,
            func,
            ctx,
        }
    }

    /// 获取核心上下文的可变引用
    pub fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.ctx
    }

    /// 获取核心上下文的不可变引用
    pub fn core_ctx(&self) -> &CoreContext {
        self.ctx
    }

    /// 获取类型维度池的不可变引用
    pub fn type_dims(&self) -> &RefMap<U32OptVec, Vec<Option<u32>>> {
        &self.ctx.type_dims
    }

    pub fn function(&self) -> &Function {
        self.func
    }

    pub fn func_mut(&mut self) -> &mut Function {
        self.func
    }

    pub fn function_and_core_ctx_mut(&mut self) -> (&mut Function, &mut CoreContext) {
        (&mut self.func, self.ctx)
    }

    pub fn ins(&mut self) -> InsertBuilder<'_, &mut FuncCursor<'a>> {
        InsertBuilder::new(self)
    }
}

impl Cursor for FuncCursor<'_> {
    fn layout(&self) -> &Layout {
        &self.func.layout
    }

    fn layout_mut(&mut self) -> &mut Layout {
        &mut self.func.layout
    }
    fn position(&self) -> CursorPosition {
        self.pos
    }

    fn set_position(&mut self, pos: CursorPosition) {
        self.pos = pos;
    }
}

impl<'c> InstInserterBase<'c> for &'c mut FuncCursor<'_> {
    fn dfg(&self) -> &DataFlowGraph {
        &self.func.dfg
    }

    fn dfg_mut(&mut self) -> &mut DataFlowGraph {
        &mut self.func.dfg
    }

    fn core_ctx(&self) -> &CoreContext {
        self.ctx
    }

    fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.ctx
    }

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext) {
        (&mut self.func.dfg, self.ctx)
    }

    fn insert_built_inst(self, inst: Inst) -> &'c mut DataFlowGraph {
        self.insert_inst(inst);
        &mut self.func.dfg
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::ssa::{Type, ctx::CoreContext, function::Function};

//     #[test]
//     fn test_cursor_movement() {
//         let mut func = Function::new();
//         let mut ctx = CoreContext::new();

//         // 创建一个基本块和指令
//         let block = func.dfg.make_block();
//         let val = func.dfg.append_block_param(block, Type::Int32);

//         // 创建一条指令
//         let inst_data = InstructionData::Unary {
//             op: crate::ssa::ir::UnaryOp::Not,
//             val,
//         };
//         let inst = func.dfg.make_inst(inst_data);
//         func.dfg.make_inst_results(inst, vec![Type::Bool]);

//         // 将指令添加到布局中
//         func.layout.append_inst(inst, block);

//         // 创建游标并移动到指令
//         let mut cursor = FuncCursor::new(&mut func, &mut ctx);
//         cursor.goto_inst(inst);
//         assert_eq!(cursor.position(), CursorPosition::At(inst));
//         assert_eq!(cursor.current_inst(), Some(inst));
//         assert_eq!(cursor.current_block(), Some(block));
//     }

//     #[test]
//     fn test_cursor_insert_and_replace() {
//         let mut func = Function::new();
//         let mut ctx = CoreContext::new();

//         // 创建一个基本块和参数
//         let block = func.dfg.make_block();
//         let param1 = func.dfg.append_block_param(block, Type::Int32);
//         let param2 = func.dfg.append_block_param(block, Type::Int32);

//         // 创建一条原始指令 加法
//         let original_inst_data = InstructionData::Binary {
//             op: crate::ssa::ir::BinOp::IAdd,
//             lhs: param1,
//             rhs: param2,
//         };
//         let original_inst = func.dfg.make_inst(original_inst_data);
//         func.dfg.make_inst_results(original_inst, vec![Type::Int32]);

//         // 将指令添加到布局中
//         func.layout.append_inst(original_inst, block);

//         // 验证基本使用方式
//         {
//             // let mut pos = FuncCursor::new(func, ctx).at_inst(inst);
//             let mut pos = FuncCursor::new(&mut func, &mut ctx).at_inst(original_inst);

//             // 使用会创建实际指令的方法，而不是 iconst (常量不创建指令)
//             // 创建一个一元运算指令
//             let _new_inst = pos.ins().ineg(param1);

//             // 验证新指令被插入到正确位置
//             let insts: Vec<_> = func.layout.block_insts(block).collect();
//             assert_eq!(insts.len(), 2);
//             // 新指令被插入到原指令之前
//             assert_eq!(insts[1], original_inst); // 原指令在后面
//         }

//         // 测试替换功能
//         {
//             // pos.func.dfg.replace(inst).isub(lhs, rhs);
//             let _result_val = func.dfg.replace(original_inst).isub(param1, param2);
//             // 替换操作不会改变指令ID，只是改变指令数据

//             // 验证指令数据已被替换
//             let inst_data = func.dfg.insts.get(original_inst).unwrap();
//             match inst_data {
//                 InstructionData::Binary { op, lhs, rhs } => {
//                     assert!(matches!(op, crate::ssa::ir::BinOp::ISub));
//                     assert_eq!(*lhs, param1);
//                     assert_eq!(*rhs, param2);
//                 }
//                 _ => panic!("期望二元运算指令"),
//             }
//         }
//     }
// }
