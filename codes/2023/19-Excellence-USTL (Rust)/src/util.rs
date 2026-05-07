use std::cell::{Ref, RefCell, RefMut};
use std::collections::HashMap;
use std::fmt::{Display, Formatter, Write};
use std::hash::{Hash, Hasher};
use std::marker::PhantomData;
use std::mem;
use std::rc::Rc;
use rand::Rng;


/// Mark an empty block to indicate that the empty block has been noticed
#[macro_export]
macro_rules! allow_nothing {
    ($($arg:tt)*) => {
        // nothing
        #[allow(unused)]
        if false {}
    };
}
pub struct MutableRef<T> {
    owner: Rc<RefCell<T>>,
}

impl<T> MutableRef<T> {
    pub fn borrow_mut(&self) -> RefMut<T> {
        self.owner.borrow_mut()
    }
    pub fn borrow(&self) -> Ref<T> {
        self.owner.borrow()
    }

    pub fn new(t: T) -> MutableRef<T> {
        Self {
            owner: Rc::new(RefCell::new(t)),
        }
    }
    pub fn from_ref(t: Rc<RefCell<T>>) -> MutableRef<T> {
        MutableRef { owner: t }
    }
    pub fn clone_ref(&self) -> MutableRef<T> {
        MutableRef { owner: self.owner.clone() }
    }
}

//  对外封闭
struct LinkedList<T> {
    first: Option<Rc<RefCell<LinkedNode<T>>>>,
    last: Option<Rc<RefCell<LinkedNode<T>>>>,
}

impl<T> LinkedList<T> {
    fn new() -> LinkedList<T> {
        LinkedList { first: None, last: None }
    }

    fn push(&mut self, mut node: LinkedNode<T>) -> Rc<RefCell<LinkedNode<T>>> {
        let new_last = match &self.first {
            None => {
                let node = Rc::new(RefCell::new(node));
                self.first = Some(node.clone());
                self.last = Some(node.clone());
                node
            }
            Some(_) => match &mut self.last {
                None => {
                    panic!("NEVER OCCUR")
                }
                Some(last_node) => {
                    node.prev = Some(last_node.clone());
                    node.next = None;
                    let mut last_node_mut = last_node.borrow_mut();
                    let new_last = Rc::new(RefCell::new(node));
                    last_node_mut.next = Some(new_last.clone());
                    new_last
                }
            },
        };
        self.last = Some(new_last.clone());
        new_last
    }
}

pub struct LinkedNode<T> {
    pub value: T,
    pub hash_help: usize,
    prev: Option<Rc<RefCell<LinkedNode<T>>>>,
    next: Option<Rc<RefCell<LinkedNode<T>>>>,
}

impl<T> LinkedNode<T> {
    fn new(value: T, hash_help: usize) -> LinkedNode<T> {
        LinkedNode {
            value,
            hash_help,
            prev: None,
            next: None,
        }
    }
    /// 得到该节点的索引
    pub fn get_node_ref(&self) -> ElementRef<T> {
        let ref_id = ElementRef {
            marker: Default::default(),
            node_id: self.hash_help,
        };
        ref_id
    }
}

#[derive(Debug)]
pub struct ElementRef<T> {
    marker: PhantomData<T>,
    node_id: usize,
}

impl<T> Display for ElementRef<T>{
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_str(&format!("{}",self.node_id))
    }
}

impl<T> Hash for ElementRef<T> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.node_id.hash(state)
    }
}

impl<T> PartialEq for ElementRef<T> {
    fn eq(&self, other: &Self) -> bool {
        self.node_id == other.node_id
    }
}

impl<T> Eq for ElementRef<T> {}

impl<T> Clone for ElementRef<T> {
    fn clone(&self) -> Self {
        Self {
            marker: PhantomData,
            node_id: self.node_id,
        }
    }
}

pub struct HashInsertVec<T> {
    vec: LinkedList<T>,
    now_node_id: usize,
    len: usize,
    nodes: HashMap<ElementRef<T>, Rc<RefCell<LinkedNode<T>>>>,
}

impl<T> HashInsertVec<T> {
    pub fn new() -> HashInsertVec<T> {
        HashInsertVec {
            vec: LinkedList::new(),
            now_node_id: 0,
            len: 0,
            nodes: HashMap::new(),
        }
    }

    pub fn push(&mut self, t: T) -> ElementRef<T> {
        let ref_id = ElementRef {
            marker: Default::default(),
            node_id: self.now_node_id,
        };
        let node = self.vec.push(LinkedNode {
            value: t,
            hash_help: self.now_node_id,
            prev: None,
            next: None,
        });
        self.now_node_id += 1;
        self.len += 1;
        self.nodes.insert(ref_id.clone(), node);
        ref_id
    }

    pub fn push_with_ele_id(&mut self, ele_id: ElementRef<T>, t: T) {
        let node = self.vec.push(LinkedNode {
            value: t,
            hash_help: self.now_node_id,
            prev: None,
            next: None,
        });
        self.len += 1;
        self.nodes.insert(ele_id, node);
    }

    /// 已验证
    pub fn insert_before(&mut self, t: T, element: &ElementRef<T>) -> ElementRef<T> {
        let ref_id: ElementRef<T> = ElementRef {
            marker: Default::default(),
            node_id: self.now_node_id,
        };
        let now_node = self.nodes[element].clone();
        let new_node = Rc::new(RefCell::new(LinkedNode::new(t, self.now_node_id)));
        let mut now_node_mut = now_node.borrow_mut();

        self.now_node_id += 1;
        self.len += 1;

        let before_node = now_node_mut.prev.clone();
        match before_node {
            None => {
                // 该节点作为头节点
                now_node_mut.prev = Some(new_node.clone());

                let mut new_node_mut = new_node.borrow_mut();
                new_node_mut.next = Some(now_node.clone());

                self.vec.first = Some(new_node.clone());
                self.nodes.insert(ref_id.clone(), new_node.clone());
            }
            Some(before_node) => {
                now_node_mut.prev = Some(new_node.clone());
                let mut before_node_mut = before_node.borrow_mut();
                before_node_mut.next = Some(new_node.clone());

                let mut new_node_mut = new_node.borrow_mut();
                new_node_mut.prev = Some(before_node.clone());
                new_node_mut.next = Some(now_node.clone());

                self.nodes.insert(ref_id.clone(), new_node.clone());
            }
        }

        ref_id
    }
    /// 已验证
    pub fn insert_after(&mut self, t: T, element: &ElementRef<T>) -> ElementRef<T> {
        let ref_id: ElementRef<T> = ElementRef {
            marker: Default::default(),
            node_id: self.now_node_id,
        };
        let new_node = Rc::new(RefCell::new(LinkedNode::new(t, self.now_node_id)));
        self.now_node_id += 1;
        self.len += 1;
        let now_node = self.nodes[element].clone();
        let mut now_node_mut = now_node.borrow_mut();
        let after_node = now_node_mut.next.clone();
        match after_node {
            None => {
                // 该节点为最后的节点
                now_node_mut.next = Some(new_node.clone());

                let mut new_node_mut = new_node.borrow_mut();

                new_node_mut.next = None;
                new_node_mut.prev = Some(now_node.clone());
                self.vec.last = Some(new_node.clone());
                self.nodes.insert(ref_id.clone(), new_node.clone());
            }
            Some(after_node) => {
                now_node_mut.next = Some(new_node.clone());
                let mut before_node_mut = after_node.borrow_mut();
                before_node_mut.prev = Some(new_node.clone());
                let mut new_node_mut = new_node.borrow_mut();
                new_node_mut.next = Some(after_node.clone());
                new_node_mut.prev = Some(now_node.clone());
                self.nodes.insert(ref_id.clone(), new_node.clone());
            }
        }
        ref_id
    }

    pub fn require_ele_id(&mut self) -> ElementRef<T> {
        let ref_id: ElementRef<T> = ElementRef {
            marker: Default::default(),
            node_id: self.now_node_id,
        };
        self.now_node_id += 1;
        ref_id
    }

    /// 已验证
    /// # 一定要释放该节点的前后节点的所有可能的可变和不可变借用，如果你不知道你在干什么，不要调用
    pub fn remove(&mut self, element_ref: &ElementRef<T>) -> T {
        let ele = self.nodes.remove(element_ref).unwrap();
        let next = ele.borrow_mut().next.clone();
        let mut pre = ele.borrow_mut().prev.clone();
        if pre.is_none() {
            // 这是第一个节点
            self.vec.first = next.clone();
        } else {
            pre.as_mut().unwrap().borrow_mut().next = next.clone();
        }

        if next.is_none() {
            // 这是最后一个节点
            self.vec.last = pre;
        } else {
            next.unwrap().borrow_mut().prev = pre;
        }
        self.len -= 1;
        let node = Rc::try_unwrap(ele);
        match node {
            Ok(node) => node.into_inner().value,
            Err(ele_) => {
                let count_time = Rc::strong_count(&ele_);
                panic!("the node ref is not 1 , actual {}", count_time)
            }
        }
    }

    pub fn contains(&self, element_ref: &ElementRef<T>) -> bool {
        self.nodes.contains_key(element_ref)
    }
    /// 内部元素迭代器
    pub fn iter(&self) -> Iter<T> {
        Iter { now: self.vec.first.clone() }
    }

    /// 节点迭代器
    /// # 注意这里会Clone下一个还没有被使用的节点链表节点的引用
    pub fn node_iter(&self) -> NodeIter<T> {
        NodeIter { now: self.vec.first.clone() }
    }

    /// # 如果不清楚节点引用是否被Clone过，这个函数最好别调用
    pub fn collect_to_vec(mut self) -> Vec<T> {
        self.vec.last = None;
        drop(self.nodes); // 清除所有的额外引用

        let mut vec = Vec::new();
        let mut now = self.vec.first;
        loop {
            match now {
                None => {
                    break;
                }
                Some(node) => {
                    // 清理引用
                    let next = node.borrow_mut().next.clone();
                    match next {
                        None => {
                            // 不需要清理引用
                        }
                        Some(next) => {
                            next.borrow_mut().prev = None;
                        }
                    }

                    let node = Rc::try_unwrap(node);
                    if node.is_err() {
                        panic!("NEVER HERE")
                    }
                    match node {
                        Ok(node) => {
                            let node = node.into_inner();
                            vec.push(node.value);
                            now = node.next;
                        }
                        Err(_) => {
                            panic!("NEVER HERE")
                        }
                    }
                }
            }
        }
        vec
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn is_node_borrow(&self, ele_ref: &ElementRef<T>) -> bool {
        let ret = &self.nodes[ele_ref];
        return match ret.try_borrow() {
            Ok(_) => false,
            Err(_) => true,
        };
    }

    /// 获得 node 可变借用
    /// # 可能panic
    /// # 不建议修改节点的前后引用
    pub fn borrow_mut_uncheck(&self, ele_ref: &ElementRef<T>) -> RefMut<LinkedNode<T>> {
        let ret = &self.nodes[ele_ref];
        ret.borrow_mut()
    }
}

pub struct VecIter<T> {
    now: Option<T>,
    next: Option<Rc<RefCell<LinkedNode<T>>>>,
}

impl<T> Iterator for VecIter<T> {
    type Item = T;

    fn next(&mut self) -> Option<Self::Item> {
        match &self.now {
            None => return None,
            Some(_) => {
                let next_option = &self.next;
                match next_option {
                    None => {
                        let ret = mem::replace(&mut self.now, None);
                        return ret;
                    }
                    Some(_) => {
                        let new_now = self.next.as_ref().unwrap();
                        let mut new_now_mut = new_now.borrow_mut();
                        let new_next = new_now_mut.next.clone();
                        new_now_mut.next = None;
                        drop(new_now_mut);
                        match new_next {
                            None => {
                                let new_now = mem::replace(&mut self.next, None).unwrap();
                                let now = Rc::try_unwrap(new_now);
                                match now {
                                    Ok(now) => {
                                        let new_now_value = now.into_inner().value;
                                        let ret = mem::replace(&mut self.now, Some(new_now_value));
                                        return ret;
                                    }
                                    Err(_) => {
                                        panic!("NEVER HERE")
                                    }
                                }
                            }
                            Some(new_next_rc) => {
                                new_next_rc.borrow_mut().prev = None;
                                let new_now = mem::replace(&mut self.next, None).unwrap();
                                let now = Rc::try_unwrap(new_now);
                                match now {
                                    Ok(now) => {
                                        let new_now_value = now.into_inner().value;
                                        let ret = mem::replace(&mut self.now, Some(new_now_value));
                                        self.next = Some(new_next_rc);
                                        return ret;
                                    }
                                    Err(_) => {
                                        panic!("NEVER HERE")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

impl<T> IntoIterator for HashInsertVec<T> {
    type Item = T;
    type IntoIter = VecIter<T>;

    fn into_iter(self) -> Self::IntoIter {
        drop(self.nodes); //清理引用
        let mut vec = self.vec;
        vec.last = None;
        let first = vec.first;
        match first {
            None => return VecIter { now: None, next: None },
            Some(now_ele_rc) => {
                let mut now_ele_mut = now_ele_rc.borrow_mut();
                let next_ele = now_ele_mut.next.clone();
                now_ele_mut.next = None; // 清理引用
                drop(now_ele_mut);
                match next_ele {
                    None => {
                        // 没有下一个节点
                        let ele = Rc::try_unwrap(now_ele_rc);
                        let node = match ele {
                            Ok(ele) => ele.into_inner(),
                            Err(_) => {
                                panic!("ERROR , ELE REF HAVE BEEN CLONE")
                            }
                        };
                        return VecIter {
                            now: Some(node.value),
                            next: None,
                        };
                    }
                    Some(next_node_rc) => {
                        let mut next = next_node_rc.borrow_mut();
                        next.prev = None; // 清理引用
                                          // 到这里，当前节点的所有Rc均已经释放
                        drop(next);
                        let ele = Rc::try_unwrap(now_ele_rc);
                        let node = match ele {
                            Ok(ele) => ele.into_inner(),
                            Err(_) => {
                                panic!("ERROR ELE REF HAVE BEEN CLONE")
                            }
                        };
                        return VecIter {
                            now: Some(node.value),
                            next: Some(next_node_rc),
                        };
                    }
                }
            }
        }
    }
}

pub struct NodeIter<T> {
    now: Option<Rc<RefCell<LinkedNode<T>>>>,
}

impl<T> Iterator for NodeIter<T> {
    type Item = Rc<RefCell<LinkedNode<T>>>;

    fn next(&mut self) -> Option<Self::Item> {
        return if self.now.is_some() {
            let mut now = self.now.clone();
            self.now = now.as_mut().unwrap().borrow().next.clone();
            now
        } else {
            None
        };
    }
}

pub struct Iter<T> {
    now: Option<Rc<RefCell<LinkedNode<T>>>>,
}

impl<T: Clone> Iterator for Iter<T> {
    type Item = T;

    fn next(&mut self) -> Option<Self::Item> {
        return if self.now.is_some() {
            let rc_now_node = self.now.as_ref().unwrap().clone();
            let now_node = rc_now_node.borrow();
            self.now = now_node.next.clone();
            Some(now_node.value.clone())
        } else {
            None
        };
    }
}

pub fn generate_random_str(length: u32) -> String {
    if length == 0 {
        panic!("error")
    }
    let mut str = String::from("");
    let mut randg = rand::thread_rng();
    for _ in 0..length {
        let num = randg.gen_range(0..26);
        let first_char = if randg.gen_bool(0.5) { 'a' } else { 'A' };
        let now_char = (num + first_char as u8) as char;
        str.push(now_char);
    }
    str
}

/// kotlin 语法 apply 的支持
pub trait Kotlin {
    fn apply_once(self, t: impl FnOnce(Self) -> Self) -> Self
    where
        Self: Sized,
    {
        t(self)
    }
    fn apply_mut(self, mut t: impl FnMut(Self) -> Self) -> Self
    where
        Self: Sized,
    {
        t(self)
    }
    fn apply(self, t: impl Fn(Self) -> Self) -> Self
    where
        Self: Sized,
    {
        t(self)
    }
}

/// kotlin apply feature import
impl<F> Kotlin for F {}

type ColoredString = String;

pub trait Colorize {
    // Font Colors
    fn red(self) -> ColoredString
        where Self: ToString + Sized { self.to_string() }
    fn green(self) -> ColoredString
        where Self: ToString + Sized, { self.to_string() }
    fn yellow(self) -> ColoredString
        where Self: ToString + Sized, { self.to_string() }
    fn blue(self) -> ColoredString
        where Self: ToString + Sized, { self.to_string() }
}

#[test]
fn test_hash_vec() {
    let mut vec = HashInsertVec::new();
    let mut tag = vec.push(-1);
    // for now in 0..100 {
    //     if now == 50 {
    //         tag = vec.push(now);
    //     } else {
    //         vec.push(now);
    //     }
    // }

    let node500 = vec.insert_after(5000, &tag);
    let node888 = vec.insert_after(888, &node500);
    let node_999 = vec.insert_before(999, &node888);

    for i in vec.iter() {
        println!("{}", i)
    }
    vec.remove(&tag);
    vec.remove(&node888);
    println!("removed");
    for i in vec.iter() {
        println!("{}", i)
    }
}

impl<'a> Colorize for &'a str {}


