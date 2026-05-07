use getset::{Getters, MutGetters};
use std::collections::HashMap;

#[derive(Debug, Clone, Default, Getters, MutGetters)]
pub struct Layout {
    #[getset(get = "pub")]
    entry_block: Option<i32>,
    #[getset(get = "pub")]
    exit_block: Option<i32>,
    #[getset(get = "pub", get_mut = "pub")]
    basic_blocks: HashMap<i32, BasicBlockNode>,
    #[getset(get = "pub", get_mut = "pub")]
    instructions: HashMap<i32, InstructionNode>,
}

#[derive(Debug, Clone, Copy, Default)]
pub struct BasicBlockNode {
    pub prev: Option<i32>,
    pub next: Option<i32>,
    pub first_inst: Option<i32>,
    pub last_inst: Option<i32>,
}

#[derive(Debug, Clone, Copy, Default)]
pub struct InstructionNode {
    pub block: Option<i32>,
    pub prev: Option<i32>,
    pub next: Option<i32>,
}

pub struct BasicBlockIter<'layout> {
    pub(crate) layout: &'layout Layout,
    pub(crate) cur: Option<i32>,
}

pub struct InstructionIter<'layout> {
    pub(crate) layout: &'layout Layout,
    pub(crate) block: Option<i32>,
    pub(crate) head: Option<i32>,
    pub(crate) tail: Option<i32>,
}

impl Layout {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn is_empty(&self) -> bool {
        self.basic_blocks.is_empty()
    }

    pub fn block_node(&self, id: i32) -> &BasicBlockNode {
        &self.basic_blocks.get(&id).unwrap()
    }

    pub fn block_iter(&self) -> BasicBlockIter {
        BasicBlockIter {
            layout: &self,
            cur: self.entry_block,
        }
    }

    pub fn inst_iter(&self, block: i32) -> InstructionIter {
        InstructionIter {
            layout: &self,
            block: Some(block),
            head: self.basic_blocks[&block].first_inst,
            tail: self.basic_blocks[&block].last_inst,
        }
    }

    pub fn next_block_of(&self, id: i32) -> Option<i32> {
        self.basic_blocks[&id].next
    }

    pub fn append_block(&mut self, id: i32) {
        assert!(self.basic_blocks.get(&id).is_none());
        self.basic_blocks.entry(id).or_insert(BasicBlockNode {
            prev: self.exit_block,
            next: None,
            first_inst: None,
            last_inst: None,
        });

        if let Some(last) = self.exit_block {
            self.basic_blocks.get_mut(&last).unwrap().next = Some(id);
        }

        self.exit_block = Some(id);

        if self.entry_block.is_none() {
            self.entry_block = Some(id)
        }
    }

    pub fn append_inst(&mut self, id: i32, block: i32) {
        assert!(self.instructions.get(&id).is_none());
        self.instructions
            .entry(id)
            .or_insert(InstructionNode {
                block: Some(block),
                prev: self.basic_blocks[&block].last_inst,
                next: None,
            })
            .block = Some(block);

        if let Some(last) = self.basic_blocks[&block].last_inst {
            self.instructions.get_mut(&last).unwrap().next = Some(id);
            self.basic_blocks.get_mut(&block).unwrap().last_inst = Some(id);
        }

        self.basic_blocks.get_mut(&block).unwrap().last_inst = Some(id);

        if self.basic_blocks[&block].first_inst.is_none() {
            self.basic_blocks.get_mut(&block).unwrap().first_inst = Some(id);
        }
    }

    pub fn insert_inst_at_start(&mut self, id: i32, block: i32) {
        self.instructions
            .entry(id)
            .or_insert(InstructionNode {
                block: Some(block),
                prev: None,
                next: self.basic_blocks[&block].first_inst,
            })
            .block = Some(block);

        if let Some(first) = self.basic_blocks[&block].first_inst {
            self.instructions.get_mut(&first).unwrap().prev = Some(id);
            self.basic_blocks.get_mut(&block).unwrap().first_inst = Some(id);
        }

        self.basic_blocks.get_mut(&block).unwrap().first_inst = Some(id);

        if self.basic_blocks[&block].last_inst.is_none() {
            self.basic_blocks.get_mut(&block).unwrap().last_inst = Some(id);
        }
    }

    pub fn remove_inst(&mut self, id: i32) {
        let block = self.instructions.get(&id).unwrap().block.unwrap();
        let prev = self.instructions.get(&id).unwrap().prev;
        let next = self.instructions.get(&id).unwrap().next;
        match prev {
            Some(prev) => {
                if let Some(instr) = self.instructions.get_mut(&prev) {
                    instr.next = next;
                }
            }
            None => {
                self.basic_blocks.get_mut(&block).unwrap().first_inst = next;
            }
        }

        match next {
            Some(next) => {
                if let Some(instr) = self.instructions.get_mut(&next) {
                    instr.prev = prev;
                }
            }
            None => {
                self.basic_blocks.get_mut(&block).unwrap().last_inst = prev;
            }
        }
        self.instructions.remove(&id);
    }
    pub fn remove_bb(&mut self, id: i32) {
        assert!(self.entry_block.unwrap() != id);
        let prev = self.basic_blocks[&id].prev;
        let next = self.basic_blocks[&id].next;
        match prev {
            Some(prev) => {
                self.basic_blocks.get_mut(&prev).unwrap().next = next;
            }
            None => (),
        }

        match next {
            Some(next) => {
                self.basic_blocks.get_mut(&next).unwrap().prev = prev;
            }
            None => {
                self.exit_block = prev;
            }
        }

        let mut cur = self.basic_blocks[&id].first_inst;
        while let Some(inst) = cur {
            self.instructions.get_mut(&inst).unwrap().block = None;
            cur = self.instructions[&inst].next;
        }
        for inst in self.inst_iter(id).collect::<Vec<_>>() {
            self.instructions.remove(&inst);
        }
        self.basic_blocks.remove(&id);
    }

    pub fn insert_inst_after(&mut self, id: i32, after_id: i32) {
        let next_inst_id_opt = self.instructions.get(&after_id).unwrap().next;
        let inst_node = InstructionNode {
            block: self.instructions[&after_id].block,
            prev: Some(after_id),
            next: next_inst_id_opt,
        };
        self.instructions.insert(id, inst_node);
        // modify prev
        if let Some(prev_node) = self.instructions.get_mut(&after_id) {
            prev_node.next = Some(id);
        }
        // modify next
        if let Some(next_inst_id) = next_inst_id_opt {
            if let Some(next_node) = self.instructions.get_mut(&next_inst_id) {
                next_node.prev = Some(id);
            }
        }
    }

    pub fn insert_inst_before(&mut self, id: i32, before_id: i32) {
        let prev_inst_id_opt = self.instructions.get(&before_id).unwrap().prev;
        let inst_node = InstructionNode {
            block: self.instructions[&before_id].block,
            prev: prev_inst_id_opt,
            next: Some(before_id),
        };
        self.instructions.insert(id, inst_node);
        // modify prev
        if let Some(prev_inst_id) = prev_inst_id_opt {
            if let Some(prev_node) = self.instructions.get_mut(&prev_inst_id) {
                prev_node.next = Some(id);
            }
        }
        // modify next
        if let Some(next_node) = self.instructions.get_mut(&before_id) {
            next_node.prev = Some(id);
        }
    }
}

impl BasicBlockNode {
    pub fn last_inst(&self) -> Option<i32> {
        self.last_inst
    }
}

impl<'layout> Iterator for BasicBlockIter<'layout> {
    type Item = i32;

    fn next(&mut self) -> Option<Self::Item> {
        let cur = self.cur?;
        self.cur = self.layout.basic_blocks[&cur].next;
        Some(cur)
    }
}

impl<'layout> Iterator for InstructionIter<'layout> {
    type Item = i32;
    fn next(&mut self) -> Option<Self::Item> {
        let cur = self.head?;
        if Some(cur)
            == self
                .layout
                .basic_blocks()
                .get(&self.block.unwrap())
                .unwrap()
                .last_inst
        {
            self.head = None;
        } else {
            self.head = self.layout.instructions[&cur].next;
        }
        Some(cur)
    }
}

impl<'layout> DoubleEndedIterator for InstructionIter<'layout> {
    fn next_back(&mut self) -> Option<Self::Item> {
        let cur = self.tail?;
        if self.head == self.tail {
            self.head = None;
            self.tail = None;
        } else {
            self.tail = self.layout.instructions[&cur].prev;
        }
        Some(cur)
    }
}
