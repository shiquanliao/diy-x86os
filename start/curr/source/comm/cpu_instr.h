#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include "types.h"

static inline uint8_t inb (uint16_t port) {
    uint8_t rv;

    // inb al, dx
    __asm__ __volatile__("inb %[p], %[v]":[v]"=a"(rv) : [p]"d"(port));
    return rv;
}

static inline uint16_t inw (uint16_t port) {
    uint16_t rv;

    // in ax, dx
    __asm__ __volatile__("in %[p], %[v]":[v]"=a"(rv) : [p]"d"(port));
    return rv;
}

static inline void outb (uint16_t port, uint8_t data) {
    // outb al, dx
    __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
}

static inline void outw (uint16_t port, uint16_t data) {
    // outb al, dx
    __asm__ __volatile__("out %[v], %[p]"::[p]"d"(port), [v]"a"(data));
}

static inline void cli (void) {
    __asm__ __volatile__("cli");
}

static inline void sti (void) {
    __asm__ __volatile__("sti");
}

static inline void lgdt (uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    }gdt;

    gdt.start31_16 = start >> 16;
    gdt.start15_0 = start & 0xFFFF;
    gdt.limit = size - 1;

    __asm__ __volatile__("lgdt %[g]"::[g]"m"(gdt));
}

static inline uint32_t read_cr0 (void) {
    uint32_t cr0;

    __asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
    return cr0;
}

static inline void write_cr0 (uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

static inline uint32_t read_cr2 (void) {
    uint32_t cr2;

    __asm__ __volatile__("mov %%cr2, %[v]":[v]"=r"(cr2));
    return cr2;
}

static inline uint32_t read_cr3 (void) {
    uint32_t cr3;

    __asm__ __volatile__("mov %%cr3, %[v]":[v]"=r"(cr3));
    return cr3;
}

static inline void write_cr3 (uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr3"::[v]"r"(v));
}

static inline uint32_t read_cr4 (void) {
    uint32_t cr4;

    __asm__ __volatile__("mov %%cr4, %[v]":[v]"=r"(cr4));
    return cr4;
}

static inline void write_cr4 (uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr4"::[v]"r"(v));
}

static inline void lidt (uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    }idt;

    idt.start31_16 = start >> 16;
    idt.start15_0 = start & 0xFFFF;
    idt.limit = size - 1;

    __asm__ __volatile__("lidt %[g]"::[g]"m"(idt));
}


static inline void far_jump (uint32_t selector, uint32_t offset) {
    uint32_t addr[] = {offset, selector};

    __asm__ __volatile("ljmpl *(%[a])"::[a]"r"(addr));
}

static inline void hlt (void) {
    __asm__ __volatile("hlt");
}

static inline void write_tr (uint16_t tss_sel) {
    __asm__ __volatile__("ltr %%ax"::"a"(tss_sel));

}

static inline uint32_t read_eflags (void) {
    uint32_t eflags;

    __asm__ __volatile__("pushf\n\tpop %%eax":"=a"(eflags));
    return eflags;
}

static inline void write_eflags (uint32_t eflags) {
    __asm__ __volatile__("push %%eax\n\tpopf"::"a"(eflags));
}

#endif
