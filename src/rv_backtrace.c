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

#include "rvbacktrace.h"

unsigned int rvstack_frame[STACK_FRAME_LEN]; // stack frame
unsigned int rvstack_frame_len; // stack frame len

extern unsigned int rvstack_frame_len; // stack frame len
void rvbacktrace(void)
{
#ifdef RV_BACKTRACE_USE_FP
    rvbacktrace_fno();
#else
    rvbacktrace_fomit();
#endif /* RV_BACKTRACE_USE_FP */
}

void rvbacktrace_addr2line(uint32_t *frame)
{
    char buffer[STACK_BUFFER_LEN];
    int offset = 0;

    for (int i = 0; i < rvstack_frame_len; i++)
    {
        offset += snprintf(buffer + offset, STACK_BUFFER_LEN - offset, "%lx ", frame[i]);
        if (offset >= STACK_BUFFER_LEN)
            break;
    }
    BACKTRACE_PRINTF("\naddr2line -e rtthread.elf -a -f %s\n", buffer); 
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

#ifdef RT_USING_FINSH
#include <string.h>

long rvb_test(int argc, char **argv) {
    int x, y, z;

    if (argc < 2)
    {
        BACKTRACE_PRINTF("Please input 'rvb_test <HARDFAULT|DIVBYZERO|UNALIGNED|ASSERT>' \n");
        return 0;
    }

    if (!strcmp(argv[1], "DIVBYZERO"))
    {
        x = 10;
        y = rt_strlen("");
        z = x / y;
        BACKTRACE_PRINTF("z:%d\n", z);
        return 0;
    }
    else if (!strcmp(argv[1], "UNALIGNED"))
    {
        volatile int * p;
        volatile int value;
        
        p = (int *) 0x00;
        value = *p;
        BACKTRACE_PRINTF("addr:0x%02X value:0x%08X\r\n", (int) p, value);

        p = (int *) 0x04;
        value = *p;
        BACKTRACE_PRINTF("addr:0x%02X value:0x%08X\r\n", (int) p, value);

        p = (int *) 0x03;
        value = *p;
        BACKTRACE_PRINTF("addr:0x%02X value:0x%08X\r\n", (int) p, value);
        
        return 0;
    }
    else if (!strcmp(argv[1], "ASSERT"))
    {
        RT_ASSERT(0);
    }
    else if (!strcmp(argv[1], "HARDFAULT"))
    {
        uint32_t a[1] = {0x1234};
        typedef void (*func)(void);
        func f = (func)&a[0];
        f();
    }
    return 0;
}
MSH_CMD_EXPORT(rvb_test, rvb_test: rvb_test <HARDFAULT|DIVBYZERO|UNALIGNED|ASSERT> );

RT_WEAK rt_err_t exception_hook(void *context) {
    volatile uint8_t _continue = 1;

    rvbacktrace();
    while (_continue == 1);

    return RT_EOK;
}

RT_WEAK void assert_hook(const char* ex, const char* func, rt_size_t line) {
    volatile uint8_t _continue = 1;
    
    rvbacktrace();
    while (_continue == 1);
}

int rt_cm_backtrace_init(void) {
    static rt_bool_t is_init = RT_FALSE;

    if (is_init)
    {
        return 0;
    }
    
//暂时在RISCV平台没有对应的接口，在trap_entry 或者hardfault中断中自行添加RVBACKTRACE接口函数    
//rt_hw_exception_install(exception_hook);

    rt_assert_set_hook(assert_hook);
    
    is_init = RT_TRUE;
    return 0;
}
INIT_DEVICE_EXPORT(rt_cm_backtrace_init);

#endif
