//! Layout 每个函数的布局管理器
//!
//! 由于当前布局已经十分复杂，因此独立设置一个Layout用于管理
//! 等同于一个链表
//! - BlockNode 块辅助信息
//! - InstNode 指令辅助信息

use crate::prelude::*;

/// 布局管理器
#[derive(Debug, Clone)]
pub struct Layout {
    /// 指令辅助信息
    insts: RefSecMap<Inst, InstNode>,
    /// 块辅助信息
    blocks: RefSecMap<Block, BlockNode>,
    /// 第一个块
    first_block: Option<Block>,
    /// 最后一个块
    last_block: Option<Block>,
}

impl Default for Layout {
    fn default() -> Self {
        Self::new()
    }
}

impl Layout {
    /// 创建一个新的布局管理器
    pub fn new() -> Self {
        Layout {
            insts: RefSecMap::new(),
            blocks: RefSecMap::new(),
            first_block: None,
            last_block: None,
        }
    }

    pub fn clear(&mut self) {
        self.insts.clear();
        self.blocks.clear();
        self.first_block = None;
        self.last_block = None;
    }
}

//==============================================================================
// 块管理
//==============================================================================

/// 块辅助信息
/// 可能需要计算逆后序并填充所有的prev-next关系
#[derive(Debug, Clone, PartialEq, Eq, Hash, Default)]
pub struct BlockNode {
    prev: Option<Block>,      // 前驱块
    next: Option<Block>,      // 后继块
    first_inst: Option<Inst>, // 第一个指令
    last_inst: Option<Inst>,  // 最后一个指令
}

/// 按照Layout迭代的BlocksIter
pub struct BlocksIter<'a> {
    layout: &'a Layout,
    next: Option<Block>,
}

impl Iterator for BlocksIter<'_> {
    type Item = Block;

    fn next(&mut self) -> Option<Block> {
        match self.next {
            Some(block) => {
                self.next = self.layout.next_block(block);
                Some(block)
            }
            None => None,
        }
    }
}

/// 用于直接对Layout进行迭代
impl<'f> IntoIterator for &'f Layout {
    type Item = Block;
    type IntoIter = BlocksIter<'f>;

    fn into_iter(self) -> BlocksIter<'f> {
        self.blocks()
    }
}

impl Layout {
    pub fn is_block_inserted(&self, block: Block) -> bool {
        Some(block) == self.first_block
            || (self.blocks.contains_key(block) && self.blocks[block].prev.is_some())
    }

    /// 在last_block后追加一个块
    pub fn append_block(&mut self, block: Block) {
        {
            let node = &mut self.blocks[block];
            node.prev = self.last_block;
            node.next = None;
        }
        if let Some(last) = self.last_block {
            self.blocks[last].next = block.into();
        } else {
            self.first_block = Some(block);
        }
        self.last_block = Some(block);
    }

    /// 在before前插入
    pub fn insert_block(&mut self, block: Block, before: Block) {
        debug_assert!(
            !self.is_block_inserted(block),
            "Cannot insert block that is already in the layout"
        );
        debug_assert!(
            self.is_block_inserted(before),
            "block Insertion point not in the layout"
        );
        let after = self.blocks[before].prev;
        {
            let node = &mut self.blocks[block];
            node.next = before.into();
            node.prev = after;
        }
        self.blocks[before].prev = block.into();
        match after {
            None => self.first_block = Some(block),
            Some(a) => self.blocks[a].next = block.into(),
        }
    }

    /// 在after后插入
    pub fn insert_block_after(&mut self, block: Block, after: Block) {
        debug_assert!(
            !self.is_block_inserted(block),
            "Cannot insert block that is already in the layout"
        );
        debug_assert!(
            self.is_block_inserted(after),
            "block Insertion point not in the layout"
        );
        let before = self.blocks[after].next;
        {
            let node = &mut self.blocks[block];
            node.next = before;
            node.prev = after.into();
        }
        self.blocks[after].next = block.into();
        match before {
            None => self.last_block = Some(block),
            Some(b) => self.blocks[b].prev = block.into(),
        }
    }

    /// 移除块
    pub fn remove_block(&mut self, block: Block) {
        debug_assert!(self.is_block_inserted(block), "block not in the layout");
        // debug_assert!(self.first_inst(block).is_none(), "block must be empty.");

        let prev;
        let next;
        {
            let n = &mut self.blocks[block];
            prev = n.prev;
            next = n.next;
            n.prev = None;
            n.next = None;
        }
        match prev {
            None => self.first_block = next,
            Some(p) => self.blocks[p].next = next,
        }
        match next {
            None => self.last_block = prev,
            Some(n) => self.blocks[n].prev = prev,
        }
    }

    /// 返回所有块的迭代器
    pub fn blocks(&self) -> BlocksIter {
        BlocksIter {
            layout: self,
            next: self.first_block,
        }
    }

    /// 入口块
    pub fn entry_block(&self) -> Option<Block> {
        self.first_block
    }

    /// 末尾块
    pub fn last_block(&self) -> Option<Block> {
        self.last_block
    }

    /// 上一个块
    pub fn prev_block(&self, block: Block) -> Option<Block> {
        self.blocks.get(block)?.prev
    }

    /// 下一个块
    pub fn next_block(&self, block: Block) -> Option<Block> {
        self.blocks.get(block)?.next
    }
}

//==============================================================================
// 指令
//==============================================================================

/// 指令辅助信息
#[derive(Debug, Clone, Copy, Default)]
pub struct InstNode {
    pub block: Option<Block>, // 所属块
    pub prev: Option<Inst>,   // 前驱指令
    pub next: Option<Inst>,   // 后继指令
}

/// 指令迭代器
pub struct InstsIter<'f> {
    layout: &'f Layout,
    head: Option<Inst>,
    tail: Option<Inst>,
}

impl Iterator for InstsIter<'_> {
    type Item = Inst;

    fn next(&mut self) -> Option<Inst> {
        let rval = self.head;
        if let Some(inst) = rval {
            if self.head == self.tail {
                self.head = None;
                self.tail = None;
            } else {
                self.head = self.layout.insts[inst].next;
            }
        }
        rval
    }
}

impl DoubleEndedIterator for InstsIter<'_> {
    fn next_back(&mut self) -> Option<Inst> {
        let rval = self.tail;
        if let Some(inst) = rval {
            if self.head == self.tail {
                self.head = None;
                self.tail = None;
            } else {
                self.tail = self.layout.insts[inst].prev;
            }
        }
        rval
    }
}

impl Layout {
    /// 获取指令所在的块
    pub fn inst_block(&self, inst: Inst) -> Option<Block> {
        self.insts.get(inst)?.block
    }

    /// 在块末尾追加指令
    pub fn append_inst(&mut self, inst: Inst, block: Block) {
        debug_assert_eq!(self.inst_block(inst), None);

        // 如果块还没有插入到布局中，自动插入它
        if !self.is_block_inserted(block) {
            self.append_block(block);
        }
        {
            let block_node = &mut self.blocks[block];
            {
                let inst_node = &mut self.insts[inst];
                inst_node.block = block.into();
                inst_node.prev = block_node.last_inst;
                debug_assert!(inst_node.next.is_none());
            }
            if block_node.first_inst.is_none() {
                block_node.first_inst = inst.into();
            } else {
                self.insts[block_node.last_inst.unwrap()].next = inst.into();
            }
            block_node.last_inst = inst.into();
        }
    }

    /// 获取块的第一个指令
    pub fn first_inst(&self, block: Block) -> Option<Inst> {
        self.blocks.get(block)?.first_inst
    }

    /// 获取块的最后一个指令
    pub fn last_inst(&self, block: Block) -> Option<Inst> {
        self.blocks.get(block)?.last_inst
    }

    /// 获取指令的后继指令
    pub fn next_inst(&self, inst: Inst) -> Option<Inst> {
        self.insts.get(inst)?.next
    }

    /// 获取指令的前驱指令
    pub fn prev_inst(&self, inst: Inst) -> Option<Inst> {
        self.insts.get(inst)?.prev
    }

    /// 在指令before之前插入指令inst
    pub fn insert_inst(&mut self, inst: Inst, before: Inst) {
        debug_assert_eq!(self.inst_block(inst), None);
        let block = self
            .inst_block(before)
            .expect("Instruction before insertion point not in the layout");
        let after = self.insts[before].prev;
        {
            let inst_node = &mut self.insts[inst];
            inst_node.block = block.into();
            inst_node.next = before.into();
            inst_node.prev = after;
        }
        self.insts[before].prev = inst.into();
        match after {
            None => self.blocks[block].first_inst = inst.into(),
            Some(a) => self.insts[a].next = inst.into(),
        }
    }

    /// 移除指令
    pub fn remove_inst(&mut self, inst: Inst) {
        let block = self.inst_block(inst).expect("Instruction already removed.");
        // Clear the `inst` node and extract links.
        let prev;
        let next;
        {
            let n = &mut self.insts[inst];
            prev = n.prev;
            next = n.next;
            n.block = None;
            n.prev = None;
            n.next = None;
        }
        // Fix up links to `inst`.
        match prev {
            None => self.blocks[block].first_inst = next,
            Some(p) => self.insts[p].next = next,
        }
        match next {
            None => self.blocks[block].last_inst = prev,
            Some(n) => self.insts[n].prev = prev,
        }
    }

    /// 获取块的指令迭代器
    pub fn block_insts(&self, block: Block) -> InstsIter<'_> {
        InstsIter {
            layout: self,
            head: self.blocks[block].first_inst,
            tail: self.blocks[block].last_inst,
        }
    }

    /// 在指定指令之前拆分块
    /// ```text
    /// old_block:
    ///     i1
    ///     i2
    ///     i3 << before
    ///     i4
    /// ```
    /// becomes:
    ///
    /// ```text
    /// old_block:
    ///     i1
    ///     i2
    /// new_block:
    ///     i3 << before
    ///     i4
    /// ```
    pub fn split_block(&mut self, new_block: Block, before: Inst) {
        let old_block = self
            .inst_block(before)
            .expect("The `before` instruction must be in the layout");
        debug_assert!(!self.is_block_inserted(new_block));

        // Insert new_block after old_block.
        let next_block = self.blocks[old_block].next;
        let last_inst = self.blocks[old_block].last_inst;
        {
            let node = &mut self.blocks[new_block];
            node.prev = old_block.into();
            node.next = next_block;
            node.first_inst = before.into();
            node.last_inst = last_inst;
        }
        self.blocks[old_block].next = new_block.into();

        // Fix backwards link.
        if Some(old_block) == self.last_block {
            self.last_block = Some(new_block);
        } else {
            self.blocks[next_block.unwrap()].prev = new_block.into();
        }

        // Disconnect the instruction links.
        let prev_inst = self.insts[before].prev;
        self.insts[before].prev = None;
        self.blocks[old_block].last_inst = prev_inst;
        match prev_inst {
            None => self.blocks[old_block].first_inst = None,
            Some(pi) => self.insts[pi].next = None,
        }

        // Fix the instruction -> block pointers.
        let mut opt_i = Some(before);
        while let Some(i) = opt_i {
            debug_assert_eq!(self.insts[i].block, Some(old_block));
            self.insts[i].block = new_block.into();
            opt_i = self.insts[i].next;
        }
    }
}
