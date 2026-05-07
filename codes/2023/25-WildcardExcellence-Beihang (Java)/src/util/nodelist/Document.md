# 双向链表

一个有头节点和尾节点的双向链表，可以在循环中安全地删除节点，也可以方便地使用两个节点创建 SubList（划分基本块时很有用）。

## Node 链表节点

Node 的构造方法不对外公开。

API：

| 方法            | 描述                         |
| --------------- | ---------------------------- |
| get             | 获取当前节点中的数据         |
| set             | 修改当前节点中的数据         |
| next            | 获取下一个节点               |
| prev            | 获取上一个节点               |
| insertBefore    | 在当前节点之前插入元素       |
| insertAfter     | 在当前节点之后插入元素       |
| insertAllBefore | 在当前节点之前插入一系列元素 |
| insertAllAfter  | 在当前节点之后插入一系列元素 |
| remove          | 删除当前节点                 |

throw IllegalNodeAccessException 的情况：

* 对头尾节点使用 get/set
* 对头节点使用 prev / 对尾节点使用 next
* 尝试在头节点之前 / 尾节点之后插入数据
* 尝试删除头/尾节点

## NodeList 双向链表

此链表实现 `Iterable<Node<T>>` 而非 `Iterable<T>`，使用 forEach 或者增强的 for 循环时遍历的是节点而非元素。

API：

| 方法                    | 描述                                                         |
| ----------------------- | ------------------------------------------------------------ |
| Node()                  | 构造方法，构造空链表                                         |
| Node(collection)        | 构造方法，用一个集合构造链表                                 |
| **对元素的操作**        |                                                              |
| isEmpty                 | 判空                                                         |
| firstElement            | 获取第一个元素                                               |
| lastElement             | 获取最后一个元素                                             |
| addFirst                | 在列表开头插入新元素                                         |
| addLast                 | 在列表末尾插入新元素                                         |
| addAllFirst             | 在列表开头插入一系列元素                                     |
| addAllLast              | 在列表末尾插入一系列元素                                     |
| pollFirst               | 弹出链表开头的元素并返回，表空时返回 null                    |
| pollLast                | 弹出链表末尾的元素并返回，表空时返回 null                    |
| removeIf                | 接收一个 Lambda，删除所有符合条件的元素                      |
| **对节点的操作**        |                                                              |
| firstNode               | 获取第一个节点，即 headNode.next，可能为 tailNode            |
| lastNode                | 获取最后一个节点，即 tailNode.prev，可能为 headNode          |
| headNode                | 获取头结点，这不是元素节点，不要 get                         |
| tailNode                | 获取尾节点，这不是元素节点，不要 get                         |
| forEach                 | 接收一个 Lambda，遍历所有节点（而非元素）                    |
| forEachReverse          | 同上，反向遍历                                               |
| forEachRemaining        | 接收一个 Lambda，遍历指定节点及之后的节点（而非元素）        |
| forEachRemainingReverse | 同上，反向遍历                                               |
| subList                 | 返回一个 SubNodeList，是由两个端点（闭区间）组成的“视图”，不会复制两端点之间的节点。请保证，first 端点能够正向遍历到 last |

throw IllegalNodeAccessException 的情况：

* 对空表使用 firstElement 或 lastElement
* 尝试在头节点之前 / 尾节点之后插入数据
* 正向遍历 SubList 时，SubList 的尾节点已被删除，以至于找不到终止条件

注意：

* SubList 的头节点是左端点的前一个节点，尾节点是右端点的后一个节点；它们不一定是原链表的头尾节点。你可以用 next 方法手动越界来访问、修改、删除它们。所以请尽量使用增强 for 循环或 forEach 循环。

一些例子：

* 创建一个链表

    ```java
    NodeList<Integer> list = new NodeList<>();
    list.addAllLast(List.of(1, 2, 3));
    
    // 或者
    NodeList<Integer> list = new NodeList<>(List.of(1, 2, 3));
    ```

* 遍历链表

    ```java
    // 手写
    for (var p = list.firstNode(); p != list.tailNode(); p = p.next()) {
        System.out.println(p.get());
    }
    
    // 使用增强的 for 循环（推荐）
    for (var p : list) {
        System.out.println(p.get());
    }
    
    // 使用 forEach（如果你喜欢）
    list.forEach(p -> System.out.println(p.get()));
    ```

* 在循环中删除节点是安全的

    ```java
    // 删除偶数节点
    for (var p : list) {
        if (p.get() % 2 == 0) p.remove();
    }
    
    // 等价于
    list.removeIf(e -> e % 2 == 0);
    
    // 删除偶数节点的下一节点，注意避免删到 tail
    for (var p : list) {
        if (p.get() % 2 == 0 && p.next() != list.tailNode()) {
            p.next().remove();
        }
    }
    ```

* 循环中调用 insertAfter 时，小心死循环

    ```java
    // 死循环
    for (var p : list) {
        p.insertAfter(0);
    }
    
    // 解决：手写循环，手动跳过插入的节点
    // 所有偶数后插入一个 0
    for (var p = list.firstNode(); p != list.tailNode(); p = p.next()) {
        if (p.get() % 2 == 0) {
            p.insertAfter(0);
            p = p.next();
        }
    }
    ```

    