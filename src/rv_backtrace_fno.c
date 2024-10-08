/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-21     WangShun     the first version
 * 2024-08-30     WangShun     add addr2line function
 * 2024-09-19     WangShun     improvement of function formal parameter
 */

#include "../include/rvbacktrace.h"
#if defined(BACKTRACE_USE_FP)
static rt_uint32_t _rt_susrstack;
static rt_uint32_t _rt_eusrstack;
static rt_thread_t _backtrace_thread;
static rt_thread_t _backtrace_threadn;
static rt_object_t * thread_object_table = RT_NULL;
extern unsigned int rvstack_frame[STACK_FRAME_LEN]; // stack frame
extern unsigned int rvstack_frame_len; // stack frame len

static void walk_stackframe(int (*print_func)(const char *fmt, ...))
{
    rt_uint32_t num = 0;
    _backtrace_thread = rt_thread_self(); //    get current thread
    _rt_susrstack = (rt_uint32_t)(uintptr_t)_backtrace_thread->stack_addr; // stack start address
    _rt_eusrstack = (rt_uint32_t)(uintptr_t)(_backtrace_thread->stack_addr + _backtrace_thread->stack_size); // stack end address

    unsigned long sp, fp, ra, pc; // stack pointer, frame pointer, return address, program counter
    struct stackframe *frame;

    const register unsigned long current_sp __asm__("sp"); //   get current stack pointer
    sp = current_sp;
    fp = (unsigned long)__builtin_frame_address(0); //  get current frame pointer
    print_func("Current Thread Name:  %s \n", _backtrace_thread->parent.name);
    while (1)
    {
        frame = (struct stackframe *)(fp - BACKTRACE_LEN); //   get frame pointer

        if ((rt_uint32_t *)frame > (rt_uint32_t *)(uintptr_t)_rt_eusrstack)
        {
            rvstack_frame_len = num;
            return;
        }

        sp = fp;  // get stack pointer
        fp = frame->s_fp; // get frame pointer
        ra = frame->s_ra; // get return address
        pc = frame->s_ra - 4; // get program counter

        //  print stack interval, return address, program counter
        print_func("[%d]Stack interval :[0x%016lx - 0x%016lx]  ra 0x%016lx pc 0x%016lx\n", num, sp, fp, ra, pc);
        rvstack_frame[num] = pc; // save stack frame address
        num++;
    }
}

#if defined(BACKTRACE_ALL)
static void walk_stackframe_all(int (*print_func)(const char *fmt, ...))
{
    rt_uint32_t num = 0, i = 0;
    int thread_object_len = 0;
    unsigned long sp, fp, ra, pc; // stack pointer, frame pointer, return address, program counter
    struct stackframe *frame;

    thread_object_len = rt_object_get_length(RT_Object_Class_Thread);
    if (thread_object_len == RT_NULL)
    {
        return;
    }

    thread_object_table = (rt_object_t *) rt_malloc((sizeof(rt_object_t) * thread_object_len));
    RT_ASSERT(thread_object_table != RT_NULL);

    rt_object_get_pointers(RT_Object_Class_Thread, (rt_object_t *) thread_object_table, thread_object_len);

    for (i = 0; i < thread_object_len; i++)
    {
        _backtrace_threadn = (rt_thread_t) thread_object_table[i];

        if (_backtrace_threadn == (rt_thread_t) rt_thread_self())
        {
            continue;
        }

        _rt_susrstack = (rt_uint32_t) (uintptr_t) _backtrace_threadn->stack_addr; // stack start address
        _rt_eusrstack = (rt_uint32_t) (uintptr_t) (_backtrace_threadn->stack_addr + _backtrace_threadn->stack_size); // stack end address

        print_func("------------------------------Thread: %s backtrace------------------------------\r\n",
                _backtrace_threadn->parent.name);
        print_func("[%d]Thread Name:  %s \n", i, _backtrace_threadn->parent.name);
        sp = (unsigned long) _backtrace_threadn->sp;
        fp = ((rt_ubase_t *) (_backtrace_threadn->sp))[BACKTRACE_FP_POS]; // get current frame pointer
        while (1)
        {
            frame = (struct stackframe *) (fp - BACKTRACE_LEN); //   get frame pointer

            if ((rt_uint32_t *) frame > (rt_uint32_t *) (uintptr_t) _rt_eusrstack)
            {
                rvstack_frame_len = num;
                rvbacktrace_addr2line((rt_uint32_t *) &rvstack_frame[0], print_func);
                num = 0;
                break;
            }

            sp = fp;  // get stack pointer
            fp = frame->s_fp; // get frame pointer
            ra = frame->s_ra; // get return address
            pc = frame->s_ra - 4; // get program counter

            //  print stack interval, return address, program counter
            print_func("[%d]Stack interval :[0x%016lx - 0x%016lx]  ra 0x%016lx pc 0x%016lx\n", num, sp, fp, ra, pc);
            rvstack_frame[num] = pc; // save stack frame address
            num++;
        }
    }
    print_func("Thread Total Num: %d\n", thread_object_len);
}
#endif /* BACKTRACE_ALL */

//  backtrace function
void rvbacktrace_fno(int (*print_func)(const char *fmt, ...))
{
    print_func("\r\n---- RV_Backtrace Call Frame Start: ----\r\n");
    print_func("###Please consider the value of ra as accurate and the value of sp as only for reference###\n");
    print_func("------------------------------Thread: %s backtrace------------------------------\r\n", ((rt_thread_t)rt_thread_self())->parent.name);
    walk_stackframe(print_func);
    rvbacktrace_addr2line((rt_uint32_t *)&rvstack_frame[0], print_func); // addr2line function
#if defined (BACKTRACE_ALL_THREAD)
    print_func("\r\n");
    walk_stackframe_all(print_func);
#endif /* BACKTRACE_ALL_THREAD */
    print_func("---- RV_Backtrace Call Frame End:----\r\n");
    print_func("\r\n");
}

MSH_CMD_EXPORT_ALIAS(rvbacktrace_fno, rv_backtrace_all, backtrace all threads);
#endif /* BACKTRACE_USE_FP */
