#ifndef LOADER_H
#define LOADER_H

#include "comm/types.h"
#include "comm/boot_info.h"
#include "comm/utils.h"

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