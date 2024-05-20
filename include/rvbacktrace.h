#ifndef RV_BACKTRANCE_H
#define RV_BACKTRANCE_H

#if defined(CONFIG_KERNEL_RTTHREAD)
#include <aos/aos.h>
#include "aos/cli.h"
#include <stdio.h>
#include <stdint.h>
#include "rtthread.h"

#if __riscv_flen == 32
#define BACKTRACE_LEN 8
#endif

#if __riscv_flen == 64
#define BACKTRACE_LEN 16
#endif

#ifdef BACKTRACE_USE_FP
#define RV_BACKTRACE_USE_FP
#endif /* BACKTRACE_USE_FP */

struct stackframe
{
	unsigned long s_fp;
	unsigned long s_ra;
};

void rvbacktrace(void);
void rv_backtrace_fno(void);
int rv_backtrace_fomit(int (*print_func)(const char *fmt, ...));

#endif/* CONFIG_KERNEL_RTTHREAD */
#endif /* RV_BACKTRANCE_H */
