#ifndef RV_BACKTRANCE_H
#define RV_BACKTRANCE_H

// system include
#include <stdio.h>
#include <stdint.h>
#include "rtthread.h"

// user define
#define BACKTRACE_USE_FP

// system define
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
void rv_backtrace_fno(void);
int rv_backtrace_fomit(int (*print_func)(const char *fmt, ...));

#endif /* RV_BACKTRANCE_H */
