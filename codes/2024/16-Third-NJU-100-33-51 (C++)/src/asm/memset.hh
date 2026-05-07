// Sets the first @var{count} bytes of @var{s} to the constant byte
// @var{c}, returning a pointer to @var{s}.
// void memset (void *@var{s}, int @var{c}, @int @var{count})
// IR type : (PointerType, IntType, IntType) -> VoidType
#include <string>

std::string memset_code = 
                "memset:\n"
                "        blez    a2, .LBB0_3\n"
                "        add     a2, a2, a0\n"
                "        mv      a3, a0\n"
                ".LBB0_2:\n"
                "        addi    a4, a3, 1\n"
                "        sb      a1, 0(a3)\n"
                "        mv      a3, a4\n"
                "        bne     a4, a2, .LBB0_2\n"
                ".LBB0_3:\n"
                "        ret\n";
