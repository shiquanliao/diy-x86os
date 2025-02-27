#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include "types.h"

// 从端口port读取一个字节
static inline uint8_t inb(uint16_t port)
{
    uint8_t rv; // 读取结果
    __asm__ __volatile__("inb %[p], %[v]"
                         : [v] "=a"(rv)
                         : [p] "d"(port)); // 读取端口port的值到rv
    return rv;
}

// 将data写入端口port
static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ __volatile__("outb %[v], %[p]"
                         :
                         : [p] "d"(port), [v] "a"(data)); // 将data写入端口port
}

// 禁止中断
static inline void cli()
{
    __asm__ __volatile__("cli"); // 禁止中断
}

// 允许中断
static inline void sti()
{
    __asm__ __volatile__("sti"); // 允许中断
}

// 加载全局描述符表
static inline void lgdt(uint32_t start, uint32_t size)
{
    struct
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } gdt;

    gdt.start31_16 = start >> 16;
    gdt.start15_0 = start & 0xFFFF;
    gdt.limit = size - 1;

    __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

// 读取CR0寄存器
static inline uint32_t read_cr0() {
	uint32_t cr0;
	__asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
	return cr0;
}

// 写入CR0寄存器
static inline void write_cr0(uint32_t v) {
	__asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

static inline void far_jump(uint32_t selector, uint32_t offset) {
	uint32_t addr[] = {offset, selector };
	__asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}

#endif // CPU_INSTR_H