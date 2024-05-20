/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-23     WangShun     the first version
 */

#include "rvbacktrace.h"
#if defined(CONFIG_KERNEL_RTTHREAD)
extern char *__etext;
extern char *__stext;

#define BT_CHK_PC_AVAIL(pc)   ((uintptr_t)(pc) < (uintptr_t)(&__etext) \
                              && (uintptr_t)(pc) > (uintptr_t)(&__stext))

#define BT_FUNC_LIMIT   0x2000
#define BT_LVL_LIMIT    64

#define BT_PC2ADDR(pc)        ((char *)(((uintptr_t)(pc))))

/* get framesize from c ins32 */
static int riscv_backtrace_framesize_get1(unsigned int inst)
{
    unsigned int imm = 0;
    /* addi sp, sp, -im */
    if ((inst & 0x800FFFFF) == 0x80010113) {
        imm = (inst >> 20) & 0x7FF;
        imm = (~imm & 0x7FF) + 1;
        return imm >> 3;
    }

    return -1;
}

/* get framesize from c ins */
static int riscv_backtrace_framesize_get(unsigned short inst)
{
    unsigned int imm = 0;
    /* addi sp, sp, -im */
    if ((inst & 0xFF83) == 0x1101) {
        imm = (inst >> 2) & 0x1F;
        imm = (~imm & 0x1F) + 1;
        return imm >> 3;
    }

    /* c.addi16sp sp, nzuimm6<<4 */
    if ((inst & 0xFF83) == 0x7101) {
        imm = (inst >> 3) & 0x3;
        imm = (imm << 1) | ((inst >> 5) & 0x1);
        imm = (imm << 1) | ((inst >> 2) & 0x1);
        imm = (imm << 1) | ((inst >> 6) & 0x1);
        imm = ((~imm & 0x1f) + 1) << 4;
        return imm >> 3;
    }

    return -1;
}

static int riscv_backtrace_ra_offset_get1(unsigned int inst)
{
    unsigned int imm = 0;
    /* sd ra,552(sp) */
    if ((inst & 0x81FFF07F) == 0x113023) {
        imm = (inst >> 7) & 0x1F;
        imm |= ((inst >> 25) & 0x7F) << 5;
        /* The unit is size_t, So we don't have to move 3 bits to the left */
        return imm >> 3;
    }

    return -1;
}

/* get ra position in the stack */
static int riscv_backtrace_ra_offset_get(unsigned short inst)
{
    unsigned int imm = 0;
    /* c.fsdsp rs2, uimm6<<3(sp) */
    if ((inst & 0xE07F) == 0xE006) {
        imm = (inst >> 7) & 0x7;
        imm = (imm << 3) | ((inst >> 10) & 0x7);
        /* The unit is size_t, So we don't have to move 3 bits to the left */
        return imm;
    }

    return -1;
}
static char *k_int64tostr(int64_t num, char *str)
{
    char         index[] = "0123456789ABCDEF";
    unsigned int usnum   = (unsigned int)num;

    str[7] = index[usnum % 16];
    usnum /= 16;
    str[6] = index[usnum % 16];
    usnum /= 16;
    str[5] = index[usnum % 16];
    usnum /= 16;
    str[4] = index[usnum % 16];
    usnum /= 16;
    str[3] = index[usnum % 16];
    usnum /= 16;
    str[2] = index[usnum % 16];
    usnum /= 16;
    str[1] = index[usnum % 16];
    usnum /= 16;
    str[0] = index[usnum % 16];
    usnum /= 16;

    return str;
}

/* get the offset between the jump instruction and the return address */
static int backtraceFindLROffset(char *LR, int (*print_func)(const char *fmt, ...))
{
    int offset = 0;
    char *LR_indeed;
    unsigned int ins32;
    char         s_panic_call[] = "backtrace : 0x         \r\n";

    LR_indeed = BT_PC2ADDR(LR);

    /* Usually jump using the JAL instruction */
    ins32 = *(unsigned int *)(LR_indeed - 4);
    if ((ins32 & 0x3) == 0x3) {
        offset = 4;
    } else {
        offset = 2;
    }

    if (print_func != NULL) {
        k_int64tostr((int)((unsigned long)LR_indeed - offset), &s_panic_call[14]);
        print_func(s_panic_call);
    }

    return offset;
}

static int riscv_backtraceFromStack(long **pSP, char **pPC,
                                    int (*print_func)(const char *fmt, ...))
{
    char *CodeAddr = NULL;
    long  *SP      = *pSP;
    char *PC       = *pPC;
    char *LR;
    int   i;
    int   framesize;
    int   offset = 0;
    unsigned int ins32;
    unsigned short ins16;

    /* 1. scan code, find lr pushed */
    for (i = 0; i < BT_FUNC_LIMIT;) {
        /* FIXME: not accurate from bottom to up. how to judge 2 or 4byte inst */
        //CodeAddr = (char *)(((long)PC & (~0x3)) - i);
        CodeAddr = (char *)(PC - i);
        ins32 = *(unsigned int *)(CodeAddr);
        if ((ins32 & 0x3) == 0x3) {
            ins16 = *(unsigned short *)(CodeAddr - 2);
            if ((ins16 & 0x3) != 0x3) {
                i += 4;
                framesize = riscv_backtrace_framesize_get1(ins32);
                if (framesize >= 0) {
                    CodeAddr += 4;
                    break;
                }
                continue;
            }
        }
        i += 2;
        ins16 = (ins32 >> 16) & 0xffff;
        framesize = riscv_backtrace_framesize_get(ins16);
        if (framesize >= 0) {
            CodeAddr += 2;
            break;
        }
    }

    if (i == BT_FUNC_LIMIT) {
        /* error branch */
        if (print_func != NULL) {
            print_func("Backtrace fail!\r\n");
        }
        return -1;
    }

    /* 2. scan code, find ins: sd ra,24(sp) or sd ra,552(sp) */
    for (i = 0; CodeAddr + i < PC;) {
        ins32 = *(unsigned int *)(CodeAddr + i);
        if ((ins32 & 0x3) == 0x3) {
            i += 4;
            offset = riscv_backtrace_ra_offset_get1(ins32);
            if (offset >= 0) {
                break;
            }
        } else {
            i += 2;
            ins16 = ins32 & 0xffff;
            offset = riscv_backtrace_ra_offset_get(ins16);
            if (offset >= 0) {
                break;
            }
        }
    }

    /* 3. output */
    LR     = (char *) * (SP + offset);

    if (BT_CHK_PC_AVAIL(LR) == 0) {
        if (print_func != NULL) {
            print_func("End of stack backtracking\r\n");
        }
        return -1;
    }
    *pSP   = SP + framesize;
    offset = backtraceFindLROffset(LR, print_func);
    *pPC   = LR - offset;

    return offset == 0 ? 1 : 0;
}

static int backtraceFromStack(long **pSP, char **pPC,
                              int (*print_func)(const char *fmt, ...))
{
    if (BT_CHK_PC_AVAIL(*pPC) == 0) {
        return -1;
    }

    return riscv_backtraceFromStack(pSP, pPC, print_func);
}

/* get the return address of the current function */
__attribute__((always_inline)) static inline void *backtrace_get_sp(void)
{
    void *sp;
    __asm__ volatile("mv %0, sp\n" : "=r"(sp));
    return sp;
}

/* get the return address of the current function */
__attribute__((always_inline)) static inline void *backtrace_get_pc(void)
{
    void *pc;
    __asm__ volatile("auipc %0, 0\n" : "=r"(pc));
    return pc;
}

/* printf call stack
   return levels of call stack */
int rv_backtrace_fomit(int (*print_func)(const char *fmt, ...))
{
    char *PC;
    long  *SP;
    int   lvl;
    int   ret;

    if (print_func == NULL) {
        print_func = printf;
    }

    SP = backtrace_get_sp();
    PC = backtrace_get_pc();

    print_func("========== Call stack ==========\r\n");
    for (lvl = 0; lvl < BT_LVL_LIMIT; lvl++) {
        ret = backtraceFromStack(&SP, &PC, print_func);
        if (ret != 0) {
            break;
        }
    }
    print_func("==========    End     ==========\r\n");
    return lvl;
}
#endif/* CONFIG_KERNEL_RTTHREAD */
