#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
struct lock filesys_lock;

bool 
is_valid_addr(const void *vaddr)
{
	if(!vaddr)
		return false;
	if(!is_user_vaddr(vaddr))
		return false;
	if(!pagedir_get_page(thread_current()->pagedir, vaddr))
		return false;
	return true;
}

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

const int args[] = {
 	0, // halt
	1, // exit
	1, // exec
	1, // wait
	2, // create
	1, // remove
	1, // open
	1, // filesize
	3, // read
	3, // write
	2, // seek
	1, // tell
	1, // close
	1, // fibo
	4  // max_of_four_int
};

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int syscall_num = *(uint32_t*)(f->esp);
	
	for(int i=0;i<args[syscall_num];i++){
		if(!is_valid_addr(f->esp + 4 + (4 * i)))
			sys_exit(-1);
	}


	switch(syscall_num){
		case SYS_HALT:
			sys_halt();
		  break;
		case SYS_EXIT:
		  {
			  int exitcode = *(uint32_t *)(f->esp + 4);
			  sys_exit(exitcode);
	          break;
		  }
		case SYS_EXEC:
		  {
			  f->eax = (uint32_t)sys_exec((const char*)*(uint32_t *)(f->esp + 4));
		      break;
		  }
		case SYS_WAIT:
		  {
			  pid_t pid = *(uint32_t*)(f->esp + 4);
			  f->eax = (uint32_t) sys_wait(pid);
			  break;
		  }
		case SYS_CREATE:
		  {
			  const char* filename = (const char*)*(uint32_t *)(f->esp+16);
			  unsigned size = (unsigned)*(uint32_t *)(f->esp+20); 
			  f->eax = (uint32_t) sys_create(filename, size);
			  break;
		  }
		case SYS_REMOVE:
		  {
			  const char* filename = (const char*)*(uint32_t *)(f->esp+4);
			  f->eax = (uint32_t)sys_remove (filename);
			  break;
		  }
		case SYS_OPEN:
		  {
			  const char* filename = (const char*)*(uint32_t *)(f->esp+4);
			  f->eax = (uint32_t)sys_open (filename);
		  	  break;
		  }
		case SYS_FILESIZE:
		  {
			  int fd = *(uint32_t*)(f->esp + 4);
			  f->eax = (uint32_t) sys_filesize(fd);
		  	  break;
		  }
		case SYS_READ:
		  {
			  int fd = (int)*(uint32_t*)(f->esp+20);
			  void* buffer = (void*)*(uint32_t *)(f->esp + 24);
			  unsigned size = (unsigned)*((uint32_t*)(f->esp + 28));
			  if(!is_valid_addr(buffer))
				  sys_exit(-1);
			  f->eax = (uint32_t)sys_read(fd, buffer, size);
			  break;
		  }
		case SYS_WRITE:
		  {
			int fd = (int)*(uint32_t*)(f->esp+20);
			void* buffer = (void*)*(uint32_t *)(f->esp + 24);
			unsigned size = (unsigned)*((uint32_t*)(f->esp + 28));
		    if(!is_valid_addr(buffer))
			    sys_exit(-1);
			f->eax = (uint32_t)sys_write(fd, buffer, size);
		    break;
		  }
		case SYS_SEEK:
		  {
			  int fd = (int)*(uint32_t*)(f->esp+4);
			  unsigned pos = (unsigned)*(uint32_t*)(f->esp+8);
			  sys_seek(fd, pos);
		  	  break;
		  }
		case SYS_TELL:
		  {
			  int fd = (int)*(uint32_t*)(f->esp + 4);
			  f->eax = (uint32_t)sys_tell(fd);
			  break;
		  }
		case SYS_CLOSE:
		  {
			  int fd = *(uint32_t*)(f->esp + 4);
			  sys_close (fd);
		      break;
		  }
		case SYS_FIBONACCI:
		  {
			int num = *(uint32_t*)(f->esp + 4);
		  	f->eax = sys_fibonacci(num);
		  	break;
		  }
		case SYS_MAX_OF_FOUR_INT:
		  {
			int nums[4];
			for(int i=0;i<4;i++){
				nums[i] = *(int*)(f->esp + 28 + (i * 4));
			}
			f->eax = sys_max_of_four_int(nums);
		  	break;
		  }
		default:
		  break;
		
	}
}

/* ============ Here is for Project 2 ============ */


bool 
sys_create(const char *file, unsigned initial_size)
{
	if(file == NULL)
		sys_exit(-1);
	return filesys_create(file, initial_size);
}

bool 
sys_remove (const char *file)
{
	if(file == NULL)
		sys_exit(-1);
	return filesys_remove (file);
}

int 
sys_open (const char *file)
{
	if(file == NULL)
		sys_exit(-1);

	int result = -1;
	lock_acquire(&filesys_lock);
	struct file* openedFile = filesys_open(file);

	if(openedFile != NULL) {
		struct thread* t = thread_current();
		if(!strcmp(t->name, file))
			file_deny_write(openedFile);
		for(int i = 3; i < 128; i++){
			if(!(t->fd_table[i])){
				t->fd_table[i] = openedFile;
				result = i;
				break;
			}
		}
	}
	lock_release(&filesys_lock);
	return result;
}

void 
sys_close (int fd)
{
	/*
	if(fd < 3 || fd >= 131) 
		return;
*/
	struct thread* t = thread_current();
	struct file* file = (t->fd_table)[fd];
	if(file != NULL){
		(t->fd_table)[fd] = NULL;
		file_close(file);
		t->fd_using --;
	}
}

int 
sys_filesize (int fd)
{
	/*
	if(fd < 3 || fd >= 131) 
		return -1;
*/
	struct thread* t = thread_current();
	struct file* file = (t->fd_table)[fd];

	if(file == NULL) 
		sys_exit(-1);

	return file_length(file);
}

void 
sys_seek(int fd, unsigned position)
{
	/*
	if(fd < 3 || fd >= 131) 
		sys_exit(-1);
*/
	struct thread* t = thread_current();
	struct file* file = (t->fd_table)[fd];

	if(file == NULL)
		sys_exit(-1);

	file_seek(file, position);
}

unsigned 
sys_tell(int fd)
{
	/*
	if(fd < 3 || fd >= 131)
		sys_exit(-1);
*/
	struct thread* t = thread_current();
	struct file* file = (t->fd_table)[fd];

	if(file == NULL)
		sys_exit(-1);

    return file_tell(file);
}


/* ============ Below is for Project 1 & 2 ============ */

int
sys_write(int fd, const void *buffer, unsigned size)
{

	int result;

	lock_acquire(&filesys_lock);
	if(fd == 1){
		putbuf(buffer, size);
		result = size;
	}
	else if(fd > 2 && fd < 131) {
		struct thread* t = thread_current();
		struct file* file = (t->fd_table)[fd];

		if(file == NULL){
			lock_release(&filesys_lock);
			sys_exit(-1);
		}
		if(file->deny_write)
			file_deny_write(file);
		result = file_write(file, buffer, size);

	}
	else
		result = -1;
	lock_release(&filesys_lock);

	return result;
}

int
sys_read(int fd, void* buffer, unsigned size)
{

	int result;
	lock_acquire(&filesys_lock);
	if(fd == 0){
		int i;
		for(i=0;i<(int)size;i++){
			if(input_getc() == '\0') break;
		}
		result = i;
	}
	else if(fd > 2 && fd < 131) {
		struct thread* t = thread_current();
		struct file* file = (t->fd_table)[fd];
		if(file == NULL){
			lock_release(&filesys_lock);
			sys_exit(-1);
		}
		result = file_read(file, buffer, size);
	}
	else
		result = -1;
	lock_release(&filesys_lock);

	return result;
}

void
sys_exit(int exitcode)
{
	struct thread* t = thread_current();
	char* file_name = t->name;
	char *token, *next;
	
	token = strtok_r(file_name, " ", &next);
	t->exit_status = exitcode;
	printf("%s: exit(%d)\n", token, exitcode);

	for(int i=3;i<131;i++){
		if((t->fd_table)[i] != NULL){
			file_close((t->fd_table)[i]);
		}
	}

	thread_exit();
}

/* ============ Below is for Project 1 ============ */

void
sys_halt(void)
{
	shutdown_power_off();
}


pid_t
sys_exec (const char *cmd_line)
{
	return process_execute(cmd_line);
}


int 
sys_wait(pid_t pid)
{
	return process_wait(pid);
}

int 
sys_fibonacci(int num)
{
	int* nums = (int*)malloc(sizeof(int)*(num + 1));
	if(nums==NULL){
		printf("number array is not allocated\n");
		sys_exit(-1);
	}
	nums[1] = nums[2] = 1;
	for(int i=3;i<=num;i++)
		nums[i] = nums[i-1] + nums[i-2];

	int result = nums[num];
	free(nums);

	return result;
}

int
sys_max_of_four_int(int nums[])
{
	int max_1 = nums[0] > nums[1] ? nums[0] : nums[1];
	int max_2 = nums[2] > nums[3] ? nums[2] : nums[3];
	int result = max_1>max_2? max_1:max_2;
	//printf("%d\n", result);
  return result;
}
