/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-23     WangShun     the first version
 */

#include "../include/rvbacktrace.h"


void rvbacktrace(void)
{
#ifdef RV_BACKTRACE_USE_FP
    rv_backtrace_fno();
#else
    rv_backtrace_fomit(rt_kprintf);
#endif /* RV_BACKTRACE_USE_FP */
}
