/**
 * loader_16.c
 */

// 16位代码, 必须在最开始的地方加上, 以便有些io指令生成32位代码时, 不会出错
// 16位代码，必须加上放在开头，以便有些io指令生成为32位
__asm__(".code16gcc");

#include "loader.h"


static boot_info_t boot_info; // 启动参数信息

/**
 * BIOS 下显示字符
 */
static void show_msg(const char *msg)
{
    while (*msg)
    {
        __asm__ __volatile__("int $0x10"
                             :
                             : "a"(0x0e00 | *msg), "b"(0x0007));
        msg++;
    }
}

// 参考：https://wiki.osdev.org/Memory_Map_(x86)
// 1MB以下比较标准, 在1M以上会有差别
// 检测：https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_AH_.3D_0xC7
static void detect_memory(void)
{
    uint32_t contID = 0;
    SMAP_entry_t smap_entry;
    int signature, bytes;

    show_msg("try to detect memory:");

    // 初次：EDX=0x534D4150,EAX=0xE820,ECX=24,INT 0x15, EBX=0（初次）
    // 后续：EAX=0xE820,ECX=24,
    // 结束判断：EBX=0
    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++)
    {
        SMAP_entry_t *entry = &smap_entry;

        __asm__ __volatile__("int  $0x15"
                             : "=a"(signature), "=c"(bytes), "=b"(contID)
                             : "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry));
        if (signature != 0x534D4150)
        {
            show_msg("failed.\r\n");
            return;
        }

        // todo: 20字节
        // entry->ACPI & 0x0001 == 0 表示内存可用
        // bytes > 20 表示长度大于20字节
        if (bytes > 20 && (entry->ACPI & 0x0001) == 0)
        {
            continue;
        }

        // 保存RAM信息，只取32位，空间有限无需考虑更大容量的情况
        // entry->Type == 1 表示内存可用
        if (entry->Type == 1)
        {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }

        if (contID == 0)
        {
            break;
        }
    }
    show_msg("ok.\r\n");

    // 打印内存信息, size用KB
    char buf[16];
    show_msg("memory info:\r\n");
    for (int i = 0; i < boot_info.ram_region_count; i++)
    {
        show_msg("    start: ");
        uint32_to_str(boot_info.ram_region_cfg[i].start, buf);
        show_msg(buf);
        show_msg("    size: ");
        uint32_to_str(boot_info.ram_region_cfg[i].size / 1024, buf);
        show_msg(buf);
        show_msg("KB\r\n");
    }
}



void loader_entry(void)
{
    show_msg("\r\n....loading.....\r\n");
    detect_memory();
    for (;;)
    {
    }
}