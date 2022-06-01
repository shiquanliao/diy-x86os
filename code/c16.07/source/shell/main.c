/**
 * 简单的命令行解释器
 *
 * 创建时间：2021年8月5日
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include <stdio.h>
#include "lib_syscall.h"

int main (int argc, char **argv) {
    sbrk(0);
    sbrk(100);
    sbrk(200);
    sbrk(4096*2 + 200);
    sbrk(4096*5 + 1234);

    printf("ab\b\bcd\n");    // \b: 输出cd
    printf("abcd\x7f;fg\n");   // 7f: 输出 abc;fg
    printf("\0337Hello,word!\0338123\n");  // ESC 7,8 输出123lo,word!
    printf("\033[31;42mHello,word!\033[39;49m123\n");  // ESC [pn m, Hello,world红色，其余绿色

    puts("hello from x86 os");
    printf("os version: %s\n", OS_VERSION);
    puts("author: lishutong");
    puts("create data: 2022-5-31");

    puts("sh >>");

    for (int i = 0; i < argc; i++) {
        print_msg("arg: %s", (int)argv[i]);
    }

    // 创建一个自己的副本
    fork();

    sched_yield();

    for (;;) {
        print_msg("pid=%d", getpid());
        msleep(1000);
    }
}