/**
 * 功能：32位代码，完成多任务的运行
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
#include "os.h"

// --------------------------------- Macro Define ---------------------------------
/**
 * @brief 系统页表
 * 下面配置中只做了一个处理，即将0x0-4MB虚拟地址映射到0-4MB的物理地址，做恒等映射。
 */
#define MAP_ADDR (0X80000000) // 页表映射地址
#define PDE_P (1 << 0)        // 页表存在
#define PDE_W (1 << 1)        // 页表可写
#define PDE_U (1 << 2)        // 页表用户态
#define PDE_PS (1 << 7)       // 页表4MB页

// -------------------------------- Type Define --------------------------------
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// --------------------------------- External function Define ---------------------------------
// timer_init: 初始化定时器, 函数声明
void timer_init(void);
void syscall_handler(void);
void task_0(void);
void task_1(void);

// --------------------------------- Global variable Define ---------------------------------
uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36};     // align 4096: 4KB对齐, map_phy_buffer是一个页表，页表大小为4KB
static uint32_t pte_table[1024] __attribute__((aligned(4096))) = {PDE_U}; // 要给个值，否则其实始化值不确定
uint32_t pde_table[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_PS | PDE_W | PDE_U, // PDE_PS，开启4MB的页，恒等映射
};

// 用户态任务的栈
uint32_t task0_dpl0_stack[1024], task0_dpl3_stack[1024],
    task1_dpl0_stack[1024], task1_dpl3_stack[1024];

uint32_t task0_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,
    (uint32_t)task0_dpl0_stack + 4 * 1024,
    KERNEL_DATA_SEG,
    /* 后边不用使用 */ 0x0,
    0x0,
    0x0,
    0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pde_table,
    (uint32_t)task_0 /*入口地址*/,
    0x202,
    0xa,
    0xc,
    0xd,
    0xb,
    (uint32_t)task1_dpl0_stack + 4 * 1024 /* 栈 */,
    0x1,
    0x2,
    0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG,
    APP_CODE_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    0x0,
    0x0,
};

uint32_t task1_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,
    (uint32_t)task1_dpl0_stack + 4 * 1024,
    KERNEL_DATA_SEG,
    /* 后边不用使用 */ 0x0,
    0x0,
    0x0,
    0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pde_table,
    (uint32_t)task_1 /*入口地址*/,
    0x202,
    0xa,
    0xc,
    0xd,
    0xb,
    (uint32_t)task1_dpl3_stack + 4 * 1024 /* 栈 */,
    0x1,
    0x2,
    0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG,
    APP_CODE_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    APP_DATA_SEG,
    0x0,
    0x0,
};

// ldt_table: 局部描述符表
struct
{
    uint16_t offset_l, selector, attr, offset_h;
} idt_table[256] __attribute__((aligned(8))) = {1};

// gdt_table: 全局描述符表
struct
{
    uint16_t limit_l, base_l, basehl_attr, base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    // 0x00cf9a000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，32位代码段（非一致代码段），界限4G，
    [KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    // 0x00cf93000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，数据段，界限4G，可读写
    [KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf},
    // 0x00cffa000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，32位代码段，界限4G
    [APP_CODE_SEG / 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
    // 0x00cff3000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，数据段，界限4G，可读写
    [APP_DATA_SEG / 8] = {0xffff, 0x0000, 0xf300, 0x00cf},
    // 两个进程的task0和tas1的tss段:自己设置，直接写会编译报错
    [TASK0_TSS_SEL / 8] = {0x0068, 0, 0xe900, 0x0},
    [TASK1_TSS_SEL / 8] = {0x0068, 0, 0xe900, 0x0},
    // 系统调用的调用门
    [SYSCALL_SEL / 8] = {0x0000, KERNEL_CODE_SEG, 0xec03, 0x0000},
};

// --------------------------------- Function Define ---------------------------------

// outb: 向端口写入一个字节
void outb(uint8_t data, uint16_t port)
{
    // c调用汇编, 语法: __asm__ __volatile__("汇编指令" : : "寄存器"(变量));
    __asm__ __volatile__("outb %[v], %[p]"
                         :
                         : [p] "d"(port), [v] "a"(data));
}

void task_sched (void) {
    static int task_tss = TASK0_TSS_SEL;

    // 更换当前任务的tss，然后切换过去
    task_tss = (task_tss == TASK0_TSS_SEL) ? TASK1_TSS_SEL : TASK0_TSS_SEL;
    // 
    uint32_t addr[] = {0, task_tss };
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}

void do_syscall(int func, char *str, char color)
{
    static int row = 1;

    if (func == 2)
    {
        // 显示器共80列，25行，按字符显示，每个字符需要用两个字节表示
        unsigned short *dest = (unsigned short *)0xb8000 + 80 * row;
        while (*str)
        {
            // 高位表示颜色，低位表示字符
            *dest++ = *str++ | (color << 8);
        }

        // 逐行显示, 超过一行则回到第0行再显示
        row = (row >= 25) ? 0 : row + 1;

        // 加点延时, 让显示慢下来
        for (int i = 0; i < 0xFFFFFF; i++)
            ;
    }
}

void sys_show(char *str, char color)
{
    // 使用特权级0
    const unsigned long sys_gate_addr[] = {0, SYSCALL_SEL};

    // 采用调用门, 这里只支持5个参数
    // 用调用门的好处是会自动将参数复制到内核栈中，这样内核代码很好取参数
    // 而如果采用寄存器传递，取参比较困难，需要先压栈再取
    __asm__ __volatile__("push %[color];push %[str];push %[id];lcalll *(%[gate])\n\n" ::[color] "m"(color), [str] "m"(str), [id] "r"(2), [gate] "r"(sys_gate_addr));
}

/**
 * @brief 任务0
 */
void task_0(void)
{
    char *str = "task a: 1234";
    uint8_t color = 0;
    for (;;)
    {
        sys_show(str, color++);
    }
}

void task_1(void)
{
    char *str = "task b: 5678";
    uint8_t color = 0xff;
    for (;;)
    {
        sys_show(str, color--);
    }
}

void os_init(void)
{
    // 初始化8259中断控制器，打开定时器中断
    outb(0x11, 0x20);     // 开始初始化主芯片
    outb(0x11, 0xA0);     // 初始化从芯片
    outb(0x20, 0x21);     // 写ICW2，告诉主芯片中断向量从0x20开始
    outb(0x28, 0xa1);     // 写ICW2，告诉从芯片中断向量从0x28开始
    outb((1 << 2), 0x21); // 写ICW3，告诉主芯片IRQ2上连接有从芯片
    outb(2, 0xa1);        // 写ICW3，告诉从芯片连接g到主芯片的IRQ2上
    outb(0x1, 0x21);      // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式
    outb(0x1, 0xa1);      // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式
    outb(0xfe, 0x21);     // 开定时中断，其它屏幕
    outb(0xff, 0xa1);     // 屏幕所有中断

    // 设置定时器，每100ms中断一次
    int tmo = (1193180);      // 时钟频率为1193180
    outb(0x36, 0x43);         // 二进制计数、模式3、通道0
    outb((uint8_t)tmo, 0x40); // 写入低8位
    outb(tmo >> 8, 0x40);     // 写入高8位

    // 添加中断
    idt_table[0x20].offset_h = (uint32_t)timer_init >> 16;
    idt_table[0x20].offset_l = (uint32_t)timer_init & 0xffff;
    idt_table[0x20].selector = KERNEL_CODE_SEG;
    idt_table[0x20].attr = 0x8E00; // 存在，DPL=0, 中断门

    // 添加任务和系统调用
    gdt_table[TASK0_TSS_SEL / 8].base_l = (uint16_t)(uint32_t)task0_tss;
    gdt_table[TASK1_TSS_SEL / 8].base_l = (uint16_t)(uint32_t)task1_tss;
    gdt_table[SYSCALL_SEL / 8].limit_l = (uint16_t)(uint32_t)syscall_handler;

    // 虚拟内存
    // 0x80000000开始的4MB区域的映射
    pde_table[MAP_ADDR >> 22] = (uint32_t)pte_table | PDE_P | PDE_W | PDE_U;
    pte_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}