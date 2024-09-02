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

extern unsigned int rvstack_frame_len; // stack frame len
void rvbacktrace(void)
{
#ifdef RV_BACKTRACE_USE_FP
    rv_backtrace_fno();
#else
    rv_backtrace_fomit(rt_kprintf);
#endif /* RV_BACKTRACE_USE_FP */
}

void rvbacktrace_addr2line(rt_uint32_t *frame)
{
    char buffer[STACK_BUFFER_LEN];
    int offset = 0;

    for (int i = 0; i < rvstack_frame_len; i++)
    {
        offset += rt_snprintf(buffer + offset, STACK_BUFFER_LEN - offset, "%lx ", frame[i]);
        if (offset >= STACK_BUFFER_LEN)
            break;
    }
    rt_kprintf("\naddr2line -e rtthread.elf -a -f %s\n", buffer); // 打印格式化后的字符串
}
