/*
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-23     WangShun     the first version
 * 2024-09-19     WangShun     support rv32
 * 2025-04-13     Supper Thomas 修复汇编指令的异常，立即数改为8字节对齐方便适配RISCV32平台
 */

#include "rvbacktrace.h"

/* Please check that the following symbols are defined in the linked scripts ！*/ 
/* If not, define the following symbols at the beginning and end of the text segment */
extern char *BACKTRACE_TEXT_END;
extern char *BACKTRACE_TEXT_START;

extern unsigned int rvstack_frame[STACK_FRAME_LEN]; // stack frame
extern unsigned int rvstack_frame_len; // stack frame len

static int lvl;

#define BT_CHK_PC_AVAIL(pc)   ((uintptr_t)(pc) < (uintptr_t)(&BACKTRACE_TEXT_END) \
                              && (uintptr_t)(pc) > (uintptr_t)(&BACKTRACE_TEXT_START))

#define BT_FUNC_LIMIT   0x2000
#define BT_LVL_LIMIT    64

#define BT_PC2ADDR(pc)        ((char *)(((uintptr_t)(pc))))

/* get framesize from c ins32 */
static int riscv_backtrace_framesize_get1(unsigned int inst)
{
    unsigned int imm = 0;
    /* addi sp, sp, -im
     * example
     * d1010113             addi    sp,sp,-752
     * from spec addi FROM https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/  (2.4.1)
     * bit[31:20] = imm[11:0]
     * bit[19:15] = 00010
     * bit[14:12] = 000
     * bit[11:7]  = 00010
     * bit[6:0]  = 0010011
     */
    if ((inst & 0x800FFFFF) == 0x80010113) {
        imm = (inst >> 20) & 0x7FF;
        imm = (~imm & 0x7FF) + 1;
#if __riscv_xlen == 64
        return imm >> 3; // RV64: 以 8 字节为单位
#else
        return imm >> 2;  // RV32: 以 4 字节为单位
#endif
    }

    return -1;
}

/* get framesize from c ins */
static int riscv_backtrace_framesize_get(unsigned short inst)
{
    unsigned int imm = 0;
    /* addi sp, sp, -im
     * 1141:addi    sp,sp,-16
     * from spec c.addi FROM https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/
     * bit[13-15] = 000
     * bit[1:0]  = 01
     * imm[5] = bit[12]   默认负数
     * imm[4:0] = bit[6:2]
     * bit[11:7] = 00010
     * default:0x1101:  000 1 00010 00000 01
     * */
    if ((inst & 0xFF83) == 0x1101) {
        imm = (inst >> 2) & 0x1F;
        imm = (~imm & 0x1F) + 1;
#if __riscv_xlen == 64
        return imm >> 3; // RV64: 以 8 字节为单位
#else
        return imm >> 2;  // RV32: 以 4 字节为单位
#endif
    }

    /* c.addi16sp sp, nzuimm6<<4
       * 7101:addi  sp,sp,-512
       * 7119:addi  sp,sp,-128
       * from spec c.addi16sp FROM https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/
       * bit[12] = imm[9]    默认负数
       * bit[11:7] = 00010   //x2 (ra)
       * bit[6]  = imm[4]
       * bit[5]  = imm[6]
       * bit[4]  = imm[8]
       * bit[3]  = imm[7]
       * bit[2]  = imm[5]
       * bit[15:13] = 011
       * default: 0x7101: 011 1 00010 00000 01
    */
    if ((inst & 0xFF83) == 0x7101) {
        imm = (inst >> 3) & 0x3;
        imm = (imm << 1) | ((inst >> 5) & 0x1);
        imm = (imm << 1) | ((inst >> 2) & 0x1);
        imm = (imm << 1) | ((inst >> 6) & 0x1);
        imm = ((~imm & 0x1f) + 1) << 4;
#if __riscv_xlen == 64
        return imm >> 3; // RV64: 以 8 字节为单位
#else
        return imm >> 2;  // RV32: 以 4 字节为单位
#endif
    }

    return -1;
}

static int riscv_backtrace_ra_offset_get1(unsigned int inst)
{
    unsigned int imm = 0;
    /*
     * example:
     * 2e112623:sw  ra,748(sp)
     *from spec sw FROM https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/
     * BIT[31:25] = imm[11:5]
     * BIT[11:7] = imm[4:0]
     * BIT[6:0] - 0100011
     * BIT[14:12] = 010
     *
     * default: 0x1122023: 0000000 00001 00010 010 00000 0100011
    */
    if ((inst & 0x81FFF07F) == 0x112023) {
        imm = (inst >> 7) & 0x1F;
        imm |= ((inst >> 25) & 0x7F) << 5;
        /* The unit is size_t, So we don't have to move 3 bits to the left */
#if __riscv_xlen == 64
        return imm >> 3; // RV64: 以 8 字节为单位
#else
        return imm >> 2;  // RV32: 以 4 字节为单位
#endif
    }

    return -1;
}

/* get ra position in the stack */
static int riscv_backtrace_ra_offset_get(unsigned short inst)
{
    unsigned int imm = 0;
    /* sw  ra, imm(sp)
     * example:
     * c606: sw  ra,12(sp)
     * ce06: sw  ra,28(sp)
     * c206: sw  ra,4(sp)
     * c006: sw  ra,0(sp)
     * from spec c.swsp FROM https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/
     * bit[15:13] = 110
     * bit[12:9] = imm[5:2]
     * bit[8:7]  = imm[7:6]
     * bit[6:2]  = 00001
     * bit[1:0]  = 01
     * default: 0xc006 : 110 000000 00001 10
     *
     *  */
#if __riscv_xlen == 64
    if ((inst & 0xE07F) == 0xE006) {
#else
    if ((inst & 0xE07F) == 0xC006) {
#endif
        imm = (inst >> 9) & 0x0F; //imm[5:2] 已经偏移最低两位，放大了4倍
        imm = imm | (((inst >> 7) & 0x3)<<4); //imm[7:6]
        /* The unit is size_t, So we don't have to move 3 bits to the left */
#if __riscv_xlen == 64
        return imm >> 1; // RV64: 以 8 字节为单位
#else
        return imm;  // RV32: 以 4 字节为单位
#endif
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
static int backtraceFindLROffset(char *LR)
{
    int offset = 0;
    char *LR_indeed;
    unsigned int ins32;

    LR_indeed = BT_PC2ADDR(LR);

    /* Usually jump using the JAL instruction */
    //ins32 = *(unsigned int *)(LR_indeed - 4);
    rv_memcpy(&ins32, (LR_indeed - 4), sizeof(ins32));
    if ((ins32 & 0x3) == 0x3) {
        offset = 4;
    } else {
        offset = 2;
    }

    return offset;
}

static int riscv_backtraceFromStack(uint32_t **pSP, char **pPC)
{
    char *CodeAddr = NULL;
#if __riscv_xlen == 64
    uint64_t  *SP      = *pSP;
#else
    uint32_t  *SP      = *pSP;
#endif
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
        //ins32 = *(unsigned int *)(CodeAddr);
        rv_memcpy(&ins32, CodeAddr, sizeof(ins32));
        //DEBUG_RV_BACKTRACE("\n=========CodeAddr:%p==ins32:%x===",CodeAddr,ins32);
        if ((ins32 & 0x3) == 0x3) {
            ins16 = *(unsigned short *)(CodeAddr - 2);
            if ((ins16 & 0x3) != 0x3) {
                i += 4;
                framesize = riscv_backtrace_framesize_get1(ins32);
                if (framesize >= 0) {
         //           BACKTRACE_PRINTF("\n=========CodeAddr:%p==ins32:%x=framesize:%d==",CodeAddr,ins32,framesize);
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
            //DEBUG_RV_BACKTRACE("\n=========CodeAddr:%p==ins16:%x==framesize:%d=",CodeAddr,ins16,framesize);
            CodeAddr += 2;
            break;
        }
    }

    if (i == BT_FUNC_LIMIT) {
        /* error branch */
        #ifdef BACKTRACE_PRINTF
            BACKTRACE_PRINTF("Backtrace fail!\r\n");
        #endif
        return -1;
    }

    /* 2. scan code, find ins: sd ra,24(sp) or sd ra,552(sp) */
    for (i = 0; CodeAddr + i < PC;) {
        //ins32 = *(unsigned int *)(CodeAddr + i);
        rv_memcpy(&ins32, (CodeAddr+i), sizeof(ins32));
        //DEBUG_RV_BACKTRACE("\n==2=======CodeAddr:%p==ins32:%x===",CodeAddr,ins32);
        if ((ins32 & 0x3) == 0x3) {
            i += 4;
            offset = riscv_backtrace_ra_offset_get1(ins32);
            if (offset >= 0) {
                //DEBUG_RV_BACKTRACE("\n=========CodeAddr:%p==ins32:%x==offset:%d=",CodeAddr,ins32,offset);
                break;
            }
        } else {
            i += 2;
            ins16 = ins32 & 0xffff;
            offset = riscv_backtrace_ra_offset_get(ins16);
            if (offset >= 0) {
                //DEBUG_RV_BACKTRACE("\n=========CodeAddr:%p==ins16:%x==offset:%d=",CodeAddr,ins16,offset);
                break;
            }
        }
    }

    /* 3. output */
    LR     = (char *) * (SP + offset);

    if (BT_CHK_PC_AVAIL(LR) == 0) {
        #ifdef BACKTRACE_PRINTF
            BACKTRACE_PRINTF("Backtrace fail!\r\n");
        #endif
        return -1;
    }
    *pSP   = SP + framesize;
    offset = backtraceFindLROffset(LR);

    rvstack_frame[lvl] = (unsigned int)(LR - offset);

    BACKTRACE_PRINTF("[%d]Stack interval :[0x%016lx - 0x%016lx]  ra 0x%016lx pc 0x%016lx\n", lvl, SP, SP + framesize, LR, LR - offset);
    *pPC   = LR - offset;

    return offset == 0 ? 1 : 0;
}

static int backtraceFromStack(uint32_t **pSP, char **pPC)
{
    if (BT_CHK_PC_AVAIL(*pPC) == 0) {
        return -1;
    }

    return riscv_backtraceFromStack(pSP, pPC);
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
int rvbacktrace_fomit(void)
{
    char *PC;
    uint32_t  *SP;
    int   ret;

    SP = backtrace_get_sp();
    PC = backtrace_get_pc();

    BACKTRACE_PRINTF("\r\n---- RV_Backtrace Call Frame Start: -SP_size:%x---\r\n",sizeof(SP));
    BACKTRACE_PRINTF("###Please consider the value of ra as accurate and the value of sp as only for reference###\n");
    BACKTRACE_PRINTF("------------------------------Thread: %s backtrace------------------------------\r\n", ((rt_thread_t)rt_thread_self())->parent.name);
    BACKTRACE_PRINTF("----SP:0x%x----PC:0x%x----\r\n",SP,PC);
    BACKTRACE_PRINTF("----TEXT start :0x%x----end:0x%x----\r\n",&BACKTRACE_TEXT_START,&BACKTRACE_TEXT_END);
    for (lvl = 0; lvl < BT_LVL_LIMIT; lvl++) {
        ret = backtraceFromStack(&SP, &PC);
        if (ret != 0) {
            rvstack_frame_len = lvl;
            break;
        }
    }
    rvbacktrace_addr2line((uint32_t *)&rvstack_frame[0]);
    BACKTRACE_PRINTF("---- RV_Backtrace Call Frame End:----\r\n");
    BACKTRACE_PRINTF("\r\n");
    return lvl;
}
void rvbacktrace_info(uint32_t *uSP, uint32_t *uPC)
{
    char *PC;
    uint32_t  *SP;
    SP = uSP;
    PC = (char *)uPC;
    int   ret;    //先保存PC的值，方便addr2line使用
    rvstack_frame[0] = (unsigned int)(PC);
    for (lvl = 1; lvl < BT_LVL_LIMIT; lvl++) {
        ret = backtraceFromStack(&SP, &PC);
        if (ret != 0) {
            rvstack_frame_len = lvl;
            break;
        }
    }
    rvbacktrace_addr2line((uint32_t *)&rvstack_frame[0]); // addr2line function
}
#ifdef RT_USING_FINSH
void rv_backtrace_fomit_func(void)
{
    rvbacktrace_fomit();
}
MSH_CMD_EXPORT_ALIAS(rv_backtrace_fomit_func, rv_backtrace_fomit, backtrace fomit);

void rv_backtrace_hardfault_test(void)
{
    uint32_t *PC;
    uint32_t  *SP;
    int   ret;

    SP = backtrace_get_sp();
    PC = backtrace_get_pc();

    rvbacktrace_info(SP, PC);
}
MSH_CMD_EXPORT_ALIAS(rv_backtrace_hardfault_test, rv_backtrace_int_test, backtrace fomit);
#endif
