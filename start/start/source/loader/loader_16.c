/**
 * loader_16.c
 */

// 16位代码, 必须在最开始的地方加上, 以便有些io指令生成32位代码时, 不会出错
// 16位代码，必须加上放在开头，以便有些io指令生成为32位
__asm__(".code16gcc");

void loader_entry(void)
{
    unsigned char test = 'n';
    for (;;)
    {
    }
}