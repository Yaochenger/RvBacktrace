/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-21     WangShun     the first version
 */
#if defined(CONFIG_KERNEL_RTTHREAD)
#include "rvbacktrace.h"

rt_uint32_t _rt_susrstack;
rt_uint32_t _rt_eusrstack;
rt_thread_t _backtrace_thread;

static void walk_stackframe(void)
{
	rt_uint32_t num = 0;
	_backtrace_thread = rt_thread_self();
	_rt_susrstack = (rt_uint32_t)(uintptr_t)_backtrace_thread->stack_addr;
	_rt_eusrstack = (rt_uint32_t)(uintptr_t)(_backtrace_thread->stack_addr + _backtrace_thread->stack_size);

	unsigned long sp, fp, ra, pc;
	struct stackframe *frame;
	unsigned long low;
	const register unsigned long current_sp __asm__("sp");
	sp = current_sp;
	fp = (unsigned long)__builtin_frame_address(0);
	while (1)
	{
		frame = (struct stackframe *)(fp - BACKTRACE_LEN);

		if ((rt_uint32_t *)frame > (rt_uint32_t *)(uintptr_t)_rt_eusrstack)
			return;

		sp = fp;
		fp = frame->s_fp;
		ra = frame->s_ra;
		pc = frame->s_ra - 4;

		rt_kprintf("[%d]Stack interval :[0x%016lx - 0x%016lx]  ra 0x%016lx pc 0x%016lx\n", num, sp, fp, ra, pc);
		num++;
	}
}

void rv_backtrace_fno(void)
{
	rt_kprintf("---- RV_Backtrace Call Frame Start: ----\r\n");
	rt_kprintf("###Please consider the value of ra as accurate and the value of sp as only for reference###\n");
	walk_stackframe();
	rt_kprintf("---- RV_Backtrace Call Frame End:----\r\n");
}
#endif/* CONFIG_KERNEL_RTTHREAD */
