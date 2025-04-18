#ifndef RV_BACKTRANCE_H
#define RV_BACKTRANCE_H

#include <stdio.h>
#include <stdint.h>
#include "rtthread.h"

/* User Configure */
// #define BACKTRACE_USE_FP // To enable this option, add the [-fno-omit-frame-pointer] option to ASM,C/C++.
// #define BACKTRACE_ALL    // Before enabling this option, enable the BACKTRACE_USE_FP / Outputs the stack of all threads
// #define BACKTRACE_FSTACK_PROTECT // To enable this option, add the [-fstack-protector-strong] option to ASM,C/C++, add [-Wl,--wrap,_exit] flag to link option.
#define BACKTRACE_PRINTF rt_kprintf // Printf function to print stack back information
#define rv_memcpy   rt_memcpy // Memory copy function 为了支持非对齐访问

#define BACKTRACE_TEXT_START __stext 
#define BACKTRACE_TEXT_END __etext

/* Backtrace All Threads */
#if defined(BACKTRACE_USE_FP) && defined(BACKTRACE_ALL)
#define BACKTRACE_ALL_THREAD
#define BACKTRACE_FP_POS (8)
#endif /* BACKTRACE_USE_FP && defined BACKTRACE_ALL */

/* Backtrace Printf */
#if !defined(BACKTRACE_PRINTF)
#define BACKTRACE_PRINTF printf
#endif /* BACKTRACE_PRINTF */

/* User Parameter */
#define STACK_FRAME_LEN (10)
#define STACK_BUFFER_LEN (100)

#if __riscv_xlen == 32
#define BACKTRACE_LEN 8
#endif

#if __riscv_xlen == 64
#define BACKTRACE_LEN 16
#endif

#ifdef BACKTRACE_USE_FP
#define RV_BACKTRACE_USE_FP
#endif /* BACKTRACE_USE_FP */

struct stackframe
{
    unsigned long s_fp; // frame pointer
    unsigned long s_ra; // return address
};

void rvbacktrace(void); // backtrace function for usr
void rvbacktrace_fno(void);
int rvbacktrace_fomit(void);
void rvbacktrace_addr2line(uint32_t *frame);

#endif /* RV_BACKTRANCE_H */
