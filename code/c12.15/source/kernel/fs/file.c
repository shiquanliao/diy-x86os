/**
 * 文件系统相关接口的实现
 *
 * 创建时间：2021年8月5日
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "core/task.h"
#include "comm/cpu_instr.h"
#include "tools/klib.h"
#include "fs/file.h"

#define ADDR                    (8*1024*1024)      // 在0x800000处缓存原始
#define SYS_DISK_SECTOR_SIZE    512

static uint8_t * pos;       // 当前位置

static void read_disk(int sector, int sector_count, uint8_t * buf) {
	outb(0x1F1, (uint8_t) 0);
	outb(0x1F1, (uint8_t) 0);
	outb(0x1F2, (uint8_t) (sector_count >> 8));
	outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位
	outb(0x1F6, (uint8_t) (0xE0));
	outb(0x1F7, (uint8_t) 0x24);

	// 读取数据
	uint16_t *data_buf = (uint16_t*) buf;
	while (sector_count-- > 0) {
		// 每次扇区读之前都要检查，等待数据就绪
		while ((inb(0x1F7) & 0x88) != 0x8) {}

		// 读取并将数据写入到缓存中
		for (int i = 0; i < SYS_DISK_SECTOR_SIZE / 2; i++) {
			*data_buf++ = inw(0x1F0);
		}
	}
}

/**
 * 打开文件
 */
int sys_open(const char *name, int flags, ...) {
    int file = 1;

    // 暂时直接从扇区1000上读取, 读取大概40KB，足够了
    read_disk(1000, 80, (uint8_t *)ADDR);
    pos = (uint8_t *)ADDR;
    return file;
}

/**
 * 读取文件api
 */
int sys_read(int file, char *ptr, int len) {
    kernel_memcpy(ptr, pos, len);
    pos += len;
    return len;
}

/**
 * 写文件
 */
int sys_write(int file, char *ptr, int len) {
    return -1;
}

/**
 * 文件访问位置定位
 */
int sys_lseek(int file, int ptr, int dir) {
    pos = (uint8_t *)(ptr + ADDR);
    return 0;
}

/**
 * 关闭文件
 */
int sys_close(int file) {
}
