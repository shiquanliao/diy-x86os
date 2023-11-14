#ifndef LOADER_H
#define LOADER_H

#include "comm/types.h"
#include "comm/boot_info.h"
#include "comm/utils.h"
#include "comm/cpu_instr.h"


// 保护模式入口函数，在start.asm中定义
void protect_mode_entry (void);

// __attribute__((packed)) 用于告诉编译器取消结构体的字节对齐
typedef struct SMAP_entry
{
    uint32_t BaseL;
    uint32_t BaseH;
    uint32_t LengthL;
    uint32_t LengthH;
    uint32_t Type;
    uint32_t ACPI;
}
__attribute__((packed)) SMAP_entry_t;

#endif