/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-23     WangShun     the first version
 * 2024-08-30     WangShun     add addr2line function
 */

#include "../include/rvbacktrace.h"

unsigned int rvstack_frame[STACK_FRAME_LEN]; // stack frame
unsigned int rvstack_frame_len; // stack frame len

extern unsigned int rvstack_frame_len; // stack frame len
void rvbacktrace(void)
{
#ifdef RV_BACKTRACE_USE_FP
    rvbacktrace_fno(BACKTRACE_PRINTF);
#else
    rvbacktrace_fomit(BACKTRACE_PRINTF);
#endif /* RV_BACKTRACE_USE_FP */
}

void rvbacktrace_addr2line(rt_uint32_t *frame, int (*print_func)(const char *fmt, ...))
{
    char buffer[STACK_BUFFER_LEN];
    int offset = 0;

    for (int i = 0; i < rvstack_frame_len; i++)
    {
        offset += snprintf(buffer + offset, STACK_BUFFER_LEN - offset, "%lx ", frame[i]);
        if (offset >= STACK_BUFFER_LEN)
            break;
    }
    print_func("\naddr2line -e rtthread.elf -a -f %s\n", buffer); 
}

#if defined (BACKTRACE_FSTACK_PROTECT)
__attribute__ ((noreturn)) void __wrap__exit(int status)
{
    extern void rvbacktrace(void);
    extern void __rt_libc_exit(int status);
    rvbacktrace();
    __rt_libc_exit(status);
    while (1);
}
#endif /* BACKTRACE_FSTACK_PROTECT */

