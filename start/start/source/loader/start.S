    // 16位代码, 必须加上
    .code16
    .text
    .extern loader_entry
    .global _start

_start:
	// 栈和段等沿用之前的设置,也可以重新设置
	// 这里简单起见，就不做任何设置了
	// 你可能会想，直接跳到loader_entry，但这样需要先知识loader_entry在哪儿
	// boot没有这个能力做到，所以直接用汇编，以便_start处于整个bin文件开头，这样boot直接跳到开头就可以
	jmp loader_entry