#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {
	struct file *file;
	off_t ofs;	 		/* 읽어야할 파일 오프셋 */
	size_t zero_bytes; 	/* 0으로 채울 남은 페이지 바이트 */
	size_t read_bytes;	/* 가상페이지에 쓰여있는 데이터 크기 */
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
