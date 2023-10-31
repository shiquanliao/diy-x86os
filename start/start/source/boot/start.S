/**
 * 自己动手写操作系统
 *
 * 系统引导部分，启动时由硬件加载运行，然后完成对二级引导程序loader的加载
 * 该部分程序存储于磁盘的第1个扇区，在计算机启动时将会由BIOS加载到0x7c00处
 * 之后，将由BIOS跳转至0x7c00处开始运行
 * 
 * 创建时间：2021年8月5日
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
	#include "boot.h"

  	// 16位代码，务必加上
  	.code16
 	.text
	.global _start
_start:
	// 设置段寄存器
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	// 根据https://wiki.osdev.org/Memory_Map_(x86)
	// 使用0x7c00之前的空间作栈，大约有30KB的RAM，足够boot和loader使用
	mov $_start, %esp

	// 显示“Hello, OS!”
	mov $0x0e, %ah
	mov $'H', %al
	int $0x10
	mov $'e', %al
	int $0x10
	mov $'l', %al
	int $0x10
	mov $'l', %al
	int $0x10
	mov $'o', %al
	int $0x10
	mov $',', %al
	int $0x10
	mov $' ', %al
	int $0x10
	mov $'O', %al
	int $0x10
	mov $'S', %al
	int $0x10
	mov $'!', %al
	int $0x10


	jmp .
	
	// 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa
