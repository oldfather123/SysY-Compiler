import numpy as np


def init(shape, init_list):
    dim = len(shape)
    pre = shape[:]
    for i in range(1, dim):
        pre[dim - 1 - i] *= pre[dim - i]
    arr = [0] * pre[0]
    stk = []
    ind = 0
    for ch in init_list:
        if ch == '{':
            for p in pre[1:]:
                if ind % p == 0:
                    stk.append(ind + p)
                    break
            else:
                stk.append(-1)
        elif ch == '}':
            top = stk.pop()
            if top != -1:
                ind = top
        else:
            arr[ind] = int(ch)
            ind += 1
    return np.asarray(arr).reshape(shape)


print(init([3, 2, 2], "{{0}1234567}"))
print(init([2, 2, 2], "{0{1}234567}"))
print(init([3, 2, 2], "{01{2}34567}"))
print(init([2, 2, 2], "{012{3}4567}"))
print(init([3, 2, 2], "{0123{4}567}"))
print(init([2, 2, 2], "{{}1{}}"))