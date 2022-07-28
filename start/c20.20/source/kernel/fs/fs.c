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
#include "fs/fs.h"
#include "comm/boot_info.h"
#include <sys/stat.h>
#include <sys/file.h>
#include "dev/console.h"
#include "fs/file.h"
#include "tools/log.h"
#include "dev/dev.h"
#include "dev/disk.h"
#include "os_cfg.h"
#include "fs/devfs/devfs.h"
#include "fs/fatfs/fatfs.h"

#define FS_TABLE_SIZE		10		// 文件系统表数量

static list_t mounted_list;			// 已挂载的文件系统
static list_t free_list;				// 空闲fs列表
static fs_t fs_tbl[FS_TABLE_SIZE];		// 空闲文件系统列表大小
static fs_t * root_fs;				// 根文件系统

extern fs_op_t devfs_op;
extern fs_op_t fatfs_op;

#define TEMP_FILE_ID		100
#define TEMP_ADDR        	(8*1024*1024)      // 在0x800000处缓存原始

static uint8_t * temp_pos;       // 当前位置

/**
* 使用LBA48位模式读取磁盘
*/
static void read_disk(int sector, int sector_count, uint8_t * buf) {
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
    outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
    outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位

	outb(0x1F7, (uint8_t) 0x24);

	// 读取数据
	uint16_t *data_buf = (uint16_t*) buf;
	while (sector_count-- > 0) {
		// 每次扇区读之前都要检查，等待数据就绪
		while ((inb(0x1F7) & 0x88) != 0x8) {}

		// 读取并将数据写入到缓存中
		for (int i = 0; i < SECTOR_SIZE / 2; i++) {
			*data_buf++ = inw(0x1F0);
		}
	}
}

/**
 * @brief 判断文件描述符是否正确
 */
static int is_fd_bad (int file) {
	if ((file < 0) && (file >= TASK_OFILE_NR)) {
		return 1;
	}

	return 0;
}

/**
 * @brief 获取指定文件系统的操作接口
 */
static fs_op_t * get_fs_op (fs_type_t type, int major) {
	switch (type) {
	case FS_FAT16:
		return &fatfs_op;
	case FS_DEVFS:
		return &devfs_op;
	default:
		return (fs_op_t *)0;
	}
}

/**
 * @brief 挂载文件系统
 */
static fs_t * mount (fs_type_t type, char * mount_point, int dev_major, int dev_minor) {
	fs_t * fs = (fs_t *)0;

	log_printf("mount file system, name: %s, dev: %x", mount_point, dev_major);

	// 遍历，查找是否已经有挂载
 	list_node_t * curr = list_first(&mounted_list); 
	while (curr) {
		fs_t * fs = list_node_parent(curr, fs_t, node);
		if (kernel_strncmp(fs->mount_point, mount_point, FS_MOUNTP_SIZE) == 0) {
			log_printf("fs alreay mounted.");
			goto mount_failed;
		}
		curr = list_node_next(curr);
	}

	// 分配新的fs结构
	list_node_t * free_node = list_remove_first(&free_list);
	if (!free_node) {
		log_printf("no free fs, mount failed.");
		goto mount_failed;
	} 
	fs = list_node_parent(free_node, fs_t, node);

	// 检查挂载的文件系统类型：不检查实际
	fs_op_t * op = get_fs_op(type, dev_major);
	if (!op) {
		log_printf("unsupported fs type: %d", type);
		goto mount_failed;
	}

	// 给定数据一些缺省的值
	kernel_strncpy(fs->mount_point, mount_point, FS_MOUNTP_SIZE);
	fs->dev_id = 0;
	fs->op = op;
	fs->data = (void *)0;
	fs->type = FILE_UNKNOWN;

	// 挂载文件系统
	if (op->mount(fs, dev_major, dev_minor) < 0) {
		log_printf("mount fs %s failed", mount_point);
		goto mount_failed;
	}
	list_insert_last(&mounted_list, &fs->node);
	return fs;
mount_failed:
	if (fs) {
		// 回收fs
		list_insert_first(&free_list, &fs->node);
	}
	return (fs_t *)0;
}

/**
 * @brief 初始化挂载列表
 */
static void mount_list_init (void) {
	list_init(&free_list);
	for (int i = 0; i < FS_TABLE_SIZE; i++) {
		list_insert_first(&free_list, &fs_tbl[i].node);
	}
	list_init(&mounted_list);
}

/**
 * @brief 文件系统初始化
 */
void fs_init (void) {
	mount_list_init();
    file_table_init();
	
	// 磁盘检查
	disk_init();

	// 挂载根文件系统
	root_fs = mount(FS_FAT16, "/home", ROOT_DEV);
	ASSERT(root_fs != (fs_t *)0);

	// 挂载设备文件系统，待后续完成。挂载点名称可随意
	fs_t * fs = mount(FS_DEVFS, "/dev", 0, 0);
	ASSERT(fs != (fs_t *)0);
}

/**
 * @brief 目录是否有效
 */
int path_is_valid (const char * path) {
	return (path != (const char *)0) && path[0];
}

// 当前进程所在的文件系统和路径
int path_is_relative (const char * path) {
	return path_is_valid(path) && (path[0] != '/');
}

/**
 * @brief 转换目录为数字
 */
int path_to_num (const char * path, int * num) {
	int n = 0;

	const char * c = path;
	while (*c && *c != '/') {
		n = n * 10 + *c - '0';
		c++;
	}
	*num = n;
	return 0;
}

/**
 * @brief 判断路径是否以xx开头
 * 例如: /dev/tty, 以/dev开头
 */
int path_begin_with (const char * path, const char * str) {
	const char * s1 = path, * s2 = str;
	while (*s1 && *s2 && (*s1 == *s2)) {
		s1++;
		s2++;
	}

	return *s2 == '\0';
}

/**
 * @brief 获取下一级子目录
 */
const char * path_next_child (const char * path) {
   const char * c = path;

    while (*c && (*c++ == '/')) {}
    while (*c && (*c++ != '/')) {}
    return *c ? c : (const char *)0;
}

/**
 * 打开文件
 */
int sys_open(const char *name, int flags, ...) {
	// 临时使用，保留shell加载的功能
	if (kernel_strncmp(name, "/shell.elf", 4) == 0) {
        // 暂时直接从扇区1000上读取, 读取大概40KB，足够了
        read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
        temp_pos = (uint8_t *)TEMP_ADDR;
        return TEMP_FILE_ID;
    }

	// 分配文件描述符链接
	file_t * file = file_alloc();
	if (!file) {
		return -1;
	}

	int fd = task_alloc_fd(file);
	if (fd < 0) {
		goto sys_open_failed;
	}

	// 检查名称是否以挂载点开头，如果没有，则认为name在根目录下
	// 即只允许根目录下的遍历
	fs_t * fs = (fs_t *)0;
	list_node_t * node = list_first(&mounted_list);
	while (node) {
		fs_t * curr = list_node_parent(node, fs_t, node);
		if (path_begin_with(name, curr->mount_point)) {
			fs = curr;
			break;
		}
		node = list_node_next(node);
	}

	if (fs) {
		name = path_next_child(name);
	} else {
		fs = root_fs;
	}
	
	file->mode = flags;
	file->fs = fs;
	kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);
	int err = fs->op->open(fs, name, file);
	if (err < 0) {
		log_printf("open %s failed.", name);
		return -1;
	}

	return fd;

sys_open_failed:
	file_free(file);
	if (fd >= 0) {
		task_remove_fd(fd);
	}
	return -1;
}

/**
 * 复制一个文件描述符
 */
int sys_dup (int file) {
	// 超出进程所能打开的全部，退出
	if (is_fd_bad(file)) {
        log_printf("file(%d) is not valid.", file);
		return -1;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}

	int fd = task_alloc_fd(p_file);	// 新fd指向同一描述符
	if (fd >= 0) {
		file_inc_ref(p_file);
		return fd;
	}

	log_printf("No task file avaliable");
    return -1;
}

/**
 * @brief IO设备控制
 */
int sys_ioctl(int fd, int cmd, int arg0, int arg1) {
	if (is_fd_bad(fd)) {
		return 0;
	}

	file_t * pfile = task_file(fd);
	if (pfile == (file_t *)0) {
		return 0;
	}

	fs_t * fs = pfile->fs;
	return fs->op->ioctl(pfile, cmd, arg0, arg1);
}

/**
 * 读取文件api
 */
int sys_read(int file, char *ptr, int len) {
    if (file == TEMP_FILE_ID) {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }

    if (is_fd_bad(file) || !ptr || !len) {
		return 0;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}

	if (p_file->mode == O_WRONLY) {
		log_printf("file is write only");
		return -1;
	}

	// 读取文件
	fs_t * fs = p_file->fs;
	return fs->op->read(ptr, len, p_file);
}

/**
 * 写文件
 */
int sys_write(int file, char *ptr, int len) {
	if (is_fd_bad(file) || !ptr || !len) {
		return 0;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}

	if (p_file->mode == O_RDONLY) {
		log_printf("file is write only");
		return -1;
	}

	// 写入文件
	fs_t * fs = p_file->fs;
	return fs->op->write(ptr, len, p_file);
}

/**
 * 文件访问位置定位
 */
int sys_lseek(int file, int ptr, int dir) {
	// 临时保留，用于shell加载
    if (file == TEMP_FILE_ID) {
        temp_pos = (uint8_t *)(ptr + TEMP_ADDR);
        return 0;
    }

	if (is_fd_bad(file)) {
		return -1;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}

	// 写入文件
	fs_t * fs = p_file->fs;
	return fs->op->seek(p_file, ptr, dir);
}

/**
 * 关闭文件
 */
int sys_close(int file) {
    if (file == TEMP_FILE_ID) {
		return 0;
	}

	if (is_fd_bad(file)) {
		log_printf("file error");
		return -1;
	}

	file_t * p_file = task_file(file);
	if (p_file == (file_t *)0) {
		log_printf("file not opened. %d", file);
		return -1;
	}

	ASSERT(p_file->ref > 0);

	if (p_file->ref == 1) {
		fs_t * fs = p_file->fs;
		fs->op->close(p_file);
	}

	file_free(p_file);
	task_remove_fd(file);
	return 0;
}


/**
 * 判断文件描述符与tty关联
 */
int sys_isatty(int file) {
	if (is_fd_bad(file)) {
		return 0;
	}

	file_t * pfile = task_file(file);
	if (pfile == (file_t *)0) {
		return 0;
	}

	return pfile->type == FILE_TTY;
}

/**
 * @brief 获取文件状态
 */
int sys_fstat(int file, struct stat *st) {
	if (is_fd_bad(file)) {
		return -1;
	}

	file_t * p_file = task_file(file);
	if (p_file == (file_t *)0) {
		return -1;
	}

	fs_t * fs = p_file->fs;
	return fs->op->stat(p_file, st);
}

int sys_opendir(const char * name, DIR * dir) {
	return root_fs->op->opendir(root_fs, name, dir);
}

int sys_readdir(DIR* dir, struct dirent * dirent) {
	return root_fs->op->readdir(root_fs, dir, dirent);
}

int sys_closedir(DIR *dir) {
	return root_fs->op->closedir(root_fs, dir);
}

int sys_unlink (const char * path) {
	return root_fs->op->unlink(root_fs, path);
}