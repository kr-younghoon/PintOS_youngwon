/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/mmu.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "userprog/process.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		// printf("-----vm alloc page init spt find NULL-----\n");
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = (struct page *)malloc(sizeof(struct page));
		bool (*initializer)(struct page *, enum vm_type, void *);
		switch (VM_TYPE(type)){
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
		}
		uninit_new(page, upage, init, type, aux, initializer);
		// printf("-----vm alloc page init uninit new-----\n");

		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt,page);
	}
	// printf("-----vm alloc page init spt find not NULL-----\n");
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	page = (struct page *)malloc(sizeof(struct page));
	/* TODO: Fill this function. */
	page->va = pg_round_down(va);
	/* hash_find : return hash_elem */
	struct hash_elem *entry = hash_find(spt, &page->hash_elem);
	if (entry==NULL){
		// printf("------spt find page : entry is NULL----\n");
		return NULL;
	}
	// printf("------spt find page : entry is not NULL----\n");
	free(page);
	/* hash_entry : hash_elem pointer → struct pointer */
	page = hash_entry(entry, struct page, hash_elem);
	
	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	if (spt_find_page(spt, page->va) != NULL)
		return false;
	if (hash_insert(&spt->hash,&page->hash_elem)==NULL)
		succ = true;
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	void *vaddr = palloc_get_page(PAL_USER);
	if (vaddr==NULL)
		PANIC("todo");	/* page allocation failure */
	/* 할당 + 초기화 */
	frame = malloc(sizeof(struct frame));
	frame->kva = vaddr;
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	vm_alloc_page(VM_ANON | VM_MARKER_0, addr, 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (is_kernel_vaddr(addr) && !addr)
		return false;

	if (not_present){
		void *vaddr = pg_round_down(addr);
		void *rsp;
		rsp = user ? (void *)f->rsp : (void *)thread_current()->rsp;
		if (USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK)
			vm_stack_growth(vaddr);
		else if (USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK)
			vm_stack_growth(vaddr);

		return vm_claim_page(vaddr);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	/* va → page */
	page = spt_find_page(&thread_current()->spt, va);
	/* va에 해당하는 page를 찾을 수 없으면, false */
	if (page == NULL)
		return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable);
	
	return swap_in (page, frame->kva);
}

static unsigned hash_hash (const struct hash_elem *e, void *aux){
	const struct page *p = hash_entry(e, struct page, hash_elem);
	return hash_bytes (&p->va, sizeof p->va);
}
static bool hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux){
	/* Returns true if a is less than b (compare va) */
	const struct page *a_ = hash_entry(a,struct page, hash_elem);
	const struct page *b_ = hash_entry(b,struct page, hash_elem);
	return a_->va < b_->va;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash, hash_hash, hash_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	hash_apply(&src->hash, supplemental_copy_entry);
	return true;
}
void
supplemental_copy_entry(struct hash_elem *e, void *aux){
	struct page *page = hash_entry(e, struct page, hash_elem);		// hash_elem을 가지고 hash_entry를 찾아서 page에 저장

	if (page->operations->type == VM_UNINIT){
		vm_alloc_page_with_initializer(page->uninit.type, page->va, 1, page->uninit.init, page->uninit.aux);
		vm_claim_page(page->va);
	} else {
		/* 메모리에는 있는데, disk에는 없는 상태 : VM_ANON */
		vm_alloc_page(page->operations->type, page->va, 1);
		struct page *child_page = spt_find_page(&thread_current()->spt, page->va);
		vm_claim_page(page->va);									// mapping page -> frame
		memcpy(child_page->frame->kva, page->frame->kva, PGSIZE);	// 공간 할당만 해놓고 copy만 하면된다 disk에도 없기 때문에 초기화 과정이 필요없다
	}
}
/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->hash, supplemental_destory_entry);	// hash_clear : while문으로 삭제하는 함수
}
void
supplemental_destory_entry(struct hash_elem *e, void *aux){
	struct page *page = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(page);
}