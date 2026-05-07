#ifndef _STACK_H
#define _STACK_H


#define STACK_EMPTY(head) (!(head))

#define STACK_PUSH(head,add)                                         \
    STACK_PUSH2(head,add,next)

#define STACK_PUSH2(head,add,next)                                   \
do {                                                                 \
  (add)->next = (head);                                              \
  (head) = (add);                                                    \
} while (0)

#define STACK_POP(head,result)                                       \
    STACK_POP2(head,result,next)

#define STACK_POP2(head,result,next)                                 \
do {                                                                 \
  (result) = (head);                                                 \
  (head) = (head)->next;                                             \
} while (0)


#define STACK_EACH(head,el)                           \
    for ((el) = (head); el; (el) = (el)->next)

#endif