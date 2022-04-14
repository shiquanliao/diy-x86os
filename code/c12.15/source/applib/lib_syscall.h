/**
 * 系统调用接口
 *
 * 创建时间：2021年8月5日
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

// 系统调用号
#define SYS_msleep              0
#define SYS_getpid              1
#define SYS_sched_yield         3
#define SYS_fork                4
#define SYS_execve               5

int msleep (int ms);
int fork(void);
int getpid(void);
int sched_yield (void);
int execve(char *name, char **argv, char **env);

#endif //LIB_SYSCALL_H
