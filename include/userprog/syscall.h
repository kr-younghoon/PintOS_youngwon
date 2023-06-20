#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock; // 파일 동기화를 위한 전역변수

void syscall_init(void);
void close(int fd);

#endif /* userprog/syscall.h */
