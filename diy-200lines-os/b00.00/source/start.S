/**
 * 功能：16位与32位的启动混合代码
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
	#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start, timer_init

	// 当需要引用外部的符号是，使用extern声明。相当于C语言中的extern
	.extern gdt_table, pde_table, idt_table, task_0

	// 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16

	// 以下是代码区
 	.text
_start: 
	// 初始化段寄存器
	// 设置cs寄存器的值为0，这样就可以访问0x0000:0x7c00处的内存了
	mov $0, %ax			// 把0赋值给ax寄存器
	mov %ax, %ds		// 把ax寄存器的值赋值给ds寄存器
	mov %ax, %es		// 把ax寄存器的值赋值给es寄存器
	mov %ax, %ss		// 把ax寄存器的值赋值给ss寄存器
	mov %ax, %gs		// 把ax寄存器的值赋值给gs寄存器, gs(全局段寄存器)是80286引入的，用于指向全局数据段
	mov %ax, %fs		// 把ax寄存器的值赋值给fs寄存器, fs(附加段寄存器)是80386引入的，用于指向附加数据段
	mov $_start, %sp	// 把_start符号的地址赋值给sp寄存器，这样就把栈指针初始化为_start符号的地址了

_read_self_all:
	// 读取自身的全部内容到内存中
	mov $0x02, %ah		// 设置ah寄存器的值为0x02，这是读磁盘的功能号
	mov $0x40, %al		// 设置al寄存器的值为0x40，这是读磁盘的扇区数
	mov $_start_32, %bx	// 设置bx寄存器的值为_start_32符号的地址，这是读磁盘的目标地址
	mov $0x02, %cx		// 设置cx寄存器的值为0x02，这是读磁盘的扇区号
	mov $0x80, %dx		// 设置dx寄存器的值为0x80，这是读磁盘的驱动器号
	int $0x13			// 调用0x13中断，读磁盘
	jc _read_self_all	// 如果读磁盘失败，就一直重试


	// 加载ldt和gdt表, 进入保护模式
	cli					// 关闭中断
	lgdt gdt_desc		// 加载gdt描述符到gdtr寄存器
	lidt idt_desc		// 加载idt描述符到idtr寄存器
	mov $1, %eax
	lmsw %ax			// 加载cr0寄存器
	jmp $KERNEL_CODE_SEG, $_start_32	// 跳转到32位代码区

    .org 0x1fe			// 0x1fe是主引导记录的最后2个字节
	.byte 0x55, 0xaa	// 主引导记录的最后2个字节必须是0x55和0xaa

	// 32位保护模式, 位于512字节之后, 第二个扇区
	.code32				// 代码生成32位的机器指令
	.text				// 代码区
	
_start_32:
	// 重新加载数据段寄存器
	mov $KERNEL_DATA_SEG, %ax	// 设置ax寄存器的值为KERNEL_DATA_SEG，这是内核数据段的段选择子
	mov %ax, %ds				
	mov %ax, %es				
	mov %ax, %ss				
	mov $_start, %esp			// 把_start符号的地址赋值给esp寄存器，这样就把栈指针初始化为_start符号的地址了

	// 跳转到C代码
	call os_init				// 调用os_init函数

	// 打开分页机制
	mov $pde_table, %eax			// 把pg_dir符号的地址赋值给eax寄存器
	mov %eax, %cr3				// 把eax寄存器的值赋值给cr3寄存器

	mov %cr4, %eax				// 把cr4寄存器的值赋值给eax寄存器
	orl $(1 << 4), %eax			// 把eax寄存器的值或上0x10，这样就把cr4寄存器的PSE位置1了, 支持4M分页
	mov %eax, %cr4				// 把eax寄存器的值赋值给cr4寄存器

	mov %cr0, %eax				// 把cr0寄存器的值赋值给eax寄存器
	orl $(1 << 31), %eax		// 把eax寄存器的值或上0x80000000，这样就把cr0寄存器的PG位置1了, 打开分页机制
	mov %eax, %cr0				// 把eax寄存器的值赋值给cr0寄存器

	; sti							// 打开中断

	// --------------------------- 模拟中断返回, 实现从特权级0到特权级3的转换 ---------------------------
	// 以下代码用于模拟中断返回, 实现从特权级0到特权级3的转换
	// // 中断发生时，会自动压入原SS, ESP, EFLAGS, CS, EIP到栈中
	push $APP_DATA_SEG			// 把APP_DATA_SEG压入栈中, 出栈后会被加载到SS寄存器中
	push $task_dpl3_stack + 1024		// 把task_dpl3_stack压入栈中, 出栈后会被加载到ESP寄存器中
	push $0                     // 中断暂时关掉 0x202
	push $APP_CODE_SEG			// 把APP_CODE_SEG压入栈中, 出栈后会被加载到CS寄存器中
	push $task_0_entry			// 把task_0_entry压入栈中, 出栈后会被加载到EIP寄存器中
	iret						// 中断返回, 会自动从栈中弹出SS, ESP, EFLAGS, CS, EIP, 并跳转到EIP指向的地址

	jmp .

// task_0 任务的入口
task_0_entry:
	// 进入任务0时，需要重设其数据段寄存器为特权级3的
	mov %ss, %ax
	mov %ax, %ds				
	mov %ax, %es
	jmp task_0					// 跳转到task_0任务中进行执行

// 定时器0的中断处理函数, 该函数在os.c中定义
timer_init:
	push %ds					// 保存ds寄存器
	pusha						// 保存通用寄存器

	mov $0x20, %al				// 设置al寄存器的值为0x20，这是EOI的值
	outb %al, $0x20				// 发送EOI到主片

	popa						// 恢复通用寄存器
	pop %ds						// 恢复ds寄存器
	iret						// 中断返回


// gdt描述符, 由lgdt指令加载到gdtr寄存器中
gdt_desc:
	.word (256 * 8) - 1			// gdt的大小
	.long gdt_table				// gdt的起始地址, 由c代码提供

// idt描述符, 由lidt指令加载到idtr寄存器中
idt_desc:
	.word (256 * 8) - 1			// idt的大小
	.long idt_table				// idt的起始地址, 由c代码提供