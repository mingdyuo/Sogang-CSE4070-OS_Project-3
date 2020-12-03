#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"
#include "filesys/off_t.h"

struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

/* For Project 1 */
void syscall_init (void);
void sys_halt(void);
void sys_exit(int status);
int sys_write(int fd, const void *buffer, unsigned size);
pid_t sys_exec(const char *cmd_line);
int sys_read(int fd, void *buffer, unsigned size);
bool is_valid_addr(const void *ptr);
int sys_wait(pid_t pid);

int sys_fibonacci(int num);
int sys_max_of_four_int(int nums[]);

/* For Project 2 */
bool sys_create(const char *file, unsigned initial_size);
bool sys_remove (const char *file);
int sys_open (const char *file);
void sys_close (int fd);
int sys_filesize (int fd);
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);

#endif /* userprog/syscall.h */
