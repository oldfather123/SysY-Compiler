package backend.asm.instr.tags;

/**
 * 用来标记纯运算指令，即没有内存操作、改变标记位（ARM）、跳转等副作用的指令
 * 即可以参加后端 LVN 的指令
 */
public interface PureIns {
}
