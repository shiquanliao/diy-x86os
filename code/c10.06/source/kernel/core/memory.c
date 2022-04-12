/**
 * 内存管理
 *
 * 创建时间：2021年8月5日
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "core/memory.h"
#include "tools/klib.h"
#include "cpu/mmu.h"

extern uint8_t mem_free_start[];        // 内核末端空闲ram

static addr_alloc_t paddr_alloc;        // 物理地址分配结构

/**
 * @brief 初始化地址分配结构
 * 以下不检查页边界，由上层调用者检查
 */
void addr_alloc_init (addr_alloc_t * alloc, uint8_t * bitmap_bis, uint32_t start, uint32_t size, uint32_t page_size) {
    mutex_init(&paddr_alloc.mutex);

    alloc->start = start;
    alloc->size = size / page_size * page_size;     // 不够一页的丢掉
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bitmap_bis, alloc->size / page_size, 0);
}

/**
 * @brief 分配多页内存
 */
uint32_t addr_alloc_page (addr_alloc_t * alloc, int page_count) {
    uint32_t addr = 0;

    mutex_lock(&alloc->mutex);

    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count);
    if (page_index > 0) {
        addr = alloc->start + page_index * alloc->page_size;
    }

    mutex_unlock(&alloc->mutex);
    return addr;
}

/**
 * @brief 释放多页内存
 */
void addr_free_page (addr_alloc_t * alloc, uint32_t addr, int page_count) {
    mutex_lock(&alloc->mutex);

    uint32_t page_addr = (addr - alloc->start) / alloc->page_size;
    bitmap_set_bit(&alloc->bitmap, page_addr, page_count, 0);

    mutex_unlock(&alloc->mutex);
}

/**
 * @brief 获取可用的物理内存大小
 */
static uint32_t total_mem_size(boot_info_t * boot_info) {
    int mem_size = 0;
    for (int i = 0; i < boot_info->ram_region_count; i++) {
        mem_size += boot_info->ram_region_cfg[i].size;
    }

    return mem_size;
}

/**
 * @brief 将指定的地址空间进行一页的映射
 */
int memory_create_map (pde_t * page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm) {
    return 0;
} 

/**
 * @brief 根据内存映射表，构造内核页表
 */
void create_kernel_table (void) {
    extern uint8_t text_start[], text_end[], data_start[], data_end[];
    extern uint8_t kernel_base_addr[];

    // 地址映射表, 用于建立内核级的地址映射
    static memory_map_t kernel_map[] = {
        {kernel_base_addr,  text_start,     0,                  PTE_W},        // 内核栈区
        {text_start,        text_end,       text_start,         0},             // 代码区
        {data_start,        (void *)(MEM_EBDA_START - 1),   data_start,        PTE_W},      // 数据区

        // // 扩展存储空间一一映射，方便直接操作
        {(void *)MEM_EXT_START, (void *)MEM_EXT_END,     (void *)MEM_EXT_START, PTE_W},
    };

    // 清空后，然后依次根据映射关系创建映射表
    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++) {
        memory_map_t * map = kernel_map + i;

        // 可能有多个页，建立多个页的配置
        // 简化起见，不考虑4M的情况
        int vstart = down_2bound((uint32_t)map->vaddr_start, MEM_PAGE_SIZE);
        int vend = up_2bound((uint32_t)map->vaddr_end, MEM_PAGE_SIZE);
        int page_count = down_2bound((uint32_t)(vend - vstart), MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
        
        memory_create_map(0, vstart, (uint32_t)map->paddr, page_count, map->perm);
    }
}

/**
 * @brief 初始化内存管理系统
 * 该函数的主要任务：
 * 1、初始化物理内存分配器：将所有物理内存管理起来
 * 2、重新创建内核页表：原loader中创建的页表已经不再合适
 */
void memory_init (boot_info_t * boot_info) {
    // 在内核数据后面放物理页位图
    uint8_t * mem_free =(uint8_t *)mem_free_start;
    uint32_t total_mem = total_mem_size(boot_info) - MEM_EXT_START;
    total_mem = down_2bound(total_mem, MEM_PAGE_SIZE);   // 对齐到4KB页

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, total_mem, MEM_PAGE_SIZE);
    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE);

    // 后面再放点什么
    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    // 创建内核页表并切换过去
    create_kernel_table();
}
