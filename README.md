# RVBacktrace
## 简介

rv_backtrace是一个简单小型的栈回溯组件，在RISC-V架构下使用，实现了基于FP的栈回溯与非FP的栈回溯方式，通过调用统一函数接口rv_backtrace进行栈回溯，默认采用非FP方，若定义宏`BACKTRACE_USE_FP`，并添加`-fno-omit-frame-pointer`编译选项,此时采用基于FP的栈回溯，rv_backtrace可以在线程、异常处理函数中使用。（组件当前版本仅支持RT-Thread环境，可在任意支持RT-Thread的RV32上使用）

## API介绍

组件面向用户的API数量仅**1**个，组件的目的是实现RV32环境下的栈回溯，在异常或者需要的地方调用用户API即可打印出函数的调用栈，因而实现该功能1个API足以，为了保证组件的简洁性，未来也不会增加使用负担的API。

```c
void rvbacktrace(void);
```

上述API就是该组件的唯一面向用户的API，用户可以将其对接到断言函数，异常处理函数或者在任何想查看调用栈的地方使用它。

## 使用示例

注意事项：组件支持基于FP的栈回溯与非FP的栈回溯，当前默认使用基于FP的栈回溯的方式（软件暂时只支持玄铁C906,908系列），在使用时需要在汇编与C/C++的编译选项下添加编译选项（确保编译器使用fp/s0寄存器保存栈帧）：-fno-omit-frame-pointer；

这里展示在RT-Thread 支持的HPM6750 BSP下使用该组件。

```c
int main(void)
{
    app_init_led_pins();

    static uint32_t led_thread_arg = 0;
    rt_thread_t led_thread = rt_thread_create("led_th", thread_entry, &led_thread_arg, 1024, 1, 10);
    rt_thread_startup(led_thread);
    rvbacktrace(); // 在当前位置调用，将输出函数的调用栈
    return 0;
}
```

运行结果：

```c#
 \ | /
- RT -     Thread Operating System
 / | \     5.0.2 build Aug 28 2024 10:19:08
 2006 - 2022 Copyright by RT-Thread team
---- RV_Backtrace Call Frame Start: ----
###Please consider the value of ra as accurate and the value of sp as only for reference###
[0]Stack interval :[0x000000000108ea30 - 0x000000000108ea40]  ra 0x000000008000ff28 pc 0x000000008000ff24
[1]Stack interval :[0x000000000108ea40 - 0x000000000108ea50]  ra 0x000000008000fe7c pc 0x000000008000fe78
[2]Stack interval :[0x000000000108ea50 - 0x000000000108ea60]  ra 0x0000000080010002 pc 0x000000008000fffe
[3]Stack interval :[0x000000000108ea60 - 0x000000000108ea70]  ra 0x000000008000688e pc 0x000000008000688a
[4]Stack interval :[0x000000000108ea70 - 0x00000000deadbeef]  ra 0x0000000001086a7c pc 0x0000000001086a78
---- RV_Backtrace Call Frame End:----

```

上述运行结果完成将函数执行过程的程序指针pc,函数返回指针ra,函数调用栈的区间输出，当前暂时未支持符号输出，所以需要结合反汇汇编查看，生成反汇编的命令示例：

```shell
riscv32-unknown-elf-objdump -d rtthread.elf > rtthread.asm
```

## 结果分析

这里结合输出栈回溯信息与反汇编信息，依据PC查询反汇编中对应的汇编指令，列出下述分析表：

| 组件调用栈信息                                   | 反汇编信息                                                   |
| ------------------------------------------------ | ------------------------------------------------------------ |
| ra 0x000000008000ff28  pc 0x00000000**8000ff24** | **8000ff24**:	f61ff0ef          	jal	ra,8000fe84 <walk_stackframe> |
| ra 0x000000008000fe7c  pc 0x00000000**8000fe78** | **8000fe78**:	0800                	addi	s0,sp,16<br/>8000fe7a:	2049                	jal	8000fefc <rv_backtrace_fno> |
| ra 0x0000000080010002 pc 0x00000000**8000fffe**  | **8000fffe**:	e75ff0ef          	jal	ra,8000fe72 <rvbacktrace> |
| ra 0x000000008000688e pc 0x00000000**8000688a**  | **8000688a**:	738090ef          	jal	ra,8000ffc2 <main> |
| ra 0x000000000**1086a7c** pc 0x0000000001086a78  | 1086a78:	0141                	addi	sp,sp,16<br/>1086a7a:	8082                	ret<br/>**1086a7c** <_thread_exit>: |

从结果分析来看，当前组件已经初步具备实用价值，但是组件未实现对压缩指令的判断，符号解析等

## 已验证平台

|     芯片/内核     | 验证结果 |
| :---------------: | :------: |
|  D1/XuanTianC906  |   pass   |
| CH32V307/RISC-V4F |   pass   |
| HPM6750/Andes D45 |   pass   |

## Todo List 

1. 支持识别压缩指令，找到正确的PC
2. 支持输出对应函数的符号以及指令
3. 支持以图表的形式输出调用栈
4. ...

## 实现方式

Todo List  的实现方式不限与于C，ASM，Python等，能解决问题的办法就是好办法！！！

## 注意事项

1. 若系统支持跳转压缩指令，则在进行栈回溯时以`ra`的值为准，`pc`仅作为参考。