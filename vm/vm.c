// 영훈 - 6/23(금) 13:50
/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/thread.h"
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

/* (수정, 9)Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. 
 * 보류 중인 페이지 객체를 초기화하여 생성합니다. 
 * 페이지를 생성하려면 이 함수나 vm_alloc_page를 통해 직접 생성하지 마십시오.
 * page 구조체를 생성하고 적절한 초기화 함수를 설정한다.*/
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {
	// printf("vm_alloc_page_init in - vm.c:51\n");
	ASSERT (VM_TYPE(type) != VM_UNINIT)
	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. 
		upage(p를 할당할 가상 주소)가 이미 사용 중인지 확인합니다. */
	if (spt_find_page (spt, upage) == NULL) {
		// printf("upage(p를 할당할 가상 주소) == NULL vm.c:58\n");
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: 1) 페이지를 생성하고, 2) VM_type 유형에 따라 초기화 함수를 가져오세요. */
		struct page *p = (struct page *)malloc(sizeof(struct page));
		bool (*page_initializer)(struct page *, enum vm_type, void *);

		switch (VM_TYPE(type)) {
			case VM_ANON: // 페이지가 파일로 related되지 않았을때 = anon page
				page_initializer = anon_initializer;
				break;
			case VM_FILE: // 파일과 관련되었을 때
				page_initializer = file_backed_initializer;
				break;
		}
		 /* TODO: and then create "uninit" page struct by calling uninit_new. You
		  * TODO: 3) 그리고 "uninit" 페이지 구조체를 호출하여 생성하세요. */
		 uninit_new(p, upage, init, type, aux, page_initializer);
		  
		 /* TODO: should modify the field after calling the uninit_new. 
		    uninit_new를 호출한 후에 필드를 수정해야 합니다. */
		p->writable = writable;

		/* TODO: Insert the page into the spt. 
		 * 페이지를 spt에 삽입하십시오. */
		return spt_insert_page(spt, p);
	}
err:
	return false;
}

/* (수정, 5)Find VA from spt and return page. On error, return NULL. 
 * SPT에서 가상 주소(VA)를 찾고 페이지를 반환합니다. 오류가 발생한 경우 NULL을 반환합니다.
 * (GITBOOK) 인자로 넘겨진 SPT에서로부터 va와 대응되는 페이지 구조체를 찾아서 반환. 실패했을 경우 NULL return
 */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	// printf("HELLO C WORLD(sptfindpage in) - vm.c:93\n");
	struct page page;
	/* TODO: Fill this function. */
	// 인자로 받은 vaddr(va) 에 해당하는 vm_entry를 검색 후 반환
	// ㄴ 가상 메모리 주소에 해당하는 페이지 번호 추출 (pg_round_down())
	// ㄴ hash_find() 함수를 이용하여 vm_entry 검색 후 반환
 
	struct hash_elem *e;
	
	// va에 해당하는 hash_elem search
	page.va = pg_round_down(va);
	e = hash_find(&spt->spt_hash, &page.hash_elem);
	// 있으면 e에 해당하는 페이지 반환
	// ASSERT(e == NULL);
	// printf("HELLO C WORLD(sptfindpage out) - vm.c:109\n");
	return e != NULL ? hash_entry(e, struct page, hash_elem):NULL;
}

/* (수정, 4) Insert PAGE into spt with validation. 
 * 인자로 주어진 SPT에 페이지 구조체를 삽입한다. 이 함수에서 주어진 SPT에서 VA가 존재하지 않는지 검사.
 */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* TODO: Fill this function. */
	bool succ = hash_insert(&spt->spt_hash, &page->hash_elem) == NULL;
	
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

/* (수정, 6)
 * palloc() 및 프레임을 얻습니다. 
 * 사용 가능한 페이지가 없는 경우 페이지를 대체하고 반환합니다. 
 * 항상 유효한 주소를 반환합니다. 즉, 사용자 풀 메모리가 가득 찬 경우 
 * 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을 대체합니다. */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	//user pool에서 새로운 physical page를 가져온다. (palloc get page 함수를 호출.)
	void *kva = palloc_get_page(PAL_USER);

	// page 할당 실패 -> 나중에 swap_out 처리
	if (kva == NULL)
	// PANIC -> OS를 중지시키고 소스 파일명 라인 번호 함수명 등의 정보와 함께 사용자 지정 메시지를 출력.
		PANIC("todo"); // 일단 PANIC으로 해당 케이스 처리. 

	// 유저 풀에서 페이지를 성공적으로 가져오면 프레임을 할당하고 프레임 구조체의 멤버들을 초기화한 후 프레임 반환!
	frame = malloc(sizeof(struct frame)); // 프레임 할당
	frame -> kva = kva; // 프레임 멤버 초기화
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	// todo: 스택 크기를 증가시키기 위해 anon page를 하나 이상 할당하여 주어진 주소(addr)가 더 이상 예외 주소(faulted address)가 되지 않도록 합니다.
	// todo: 할당할 때 addr을 PGSIZE로 내림하여 처리
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
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
	/* TODO: Validate the fault 오류를 검증하라 */
	/* TODO: Your code goes here 코드를 작성하라 */
	// printf("fault addr: %p\n", addr);

	// not_present -> true
	if (addr == NULL)
        return false;

    if (is_kernel_vaddr(addr))
        return false;

	void *vaddr = pg_round_down(addr);

    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
        // 페이지 폴트가 스택 확장에 대한 유효한 경우인지를 확인
		//user access인 경우 rsp는 유저 stack을 가리킨다.
		//kernel access인 경우 thread에서 rsp를 가져와야한다.
		void *rsp = f->rsp;
		if (!user)
			rsp = thread_current()->rsp;
		// 스택 확장으로 처리할 수 있는 폴트인 경우, vm_stack_growth를 호출한다.
		if ((USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK) 
				|| (USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK))
			vm_stack_growth(addr);
		/* TODO: Validate the fault */
        page = spt_find_page(spt, vaddr);
        if (page == NULL)
            return false;
        if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
            return false;
		// printf("page type? %d\n", page_get_type(page));
		return vm_do_claim_page (page);
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

/* (수정, 8)Claim the page that allocate on VA.
	VA에 할당된 페이지를 차지합니다.
   ---
   1. 인자로 주어진 va에 페이지를 할당.
   2. 해당페이지에 프레임을 할당합니다.
   3. 우선 한 페이지를 얻어야 하고,
   4. return -> vm_do_claim_page(page);
 */
// 문제 없음..?
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: spt에서 va에 해당하는 page Find */
	page = spt_find_page(&thread_current() -> spt, va);
	if (page == NULL)
		return false;
	return vm_do_claim_page (page);
}

/* (수정, 7) Claim the PAGE and set up the mmu. 
 * 페이지를 요청하고(mm를 확보하고) MMU를 설정합니다.
 * 1. 인자로 주어진 page에 물리 메모리 할당.
 * 2. vm_get_frame 함수를 호출하여 프레임 1을 얻는다. (이미 스켈레톤 코드로 구현됨)
 * 3. MMU SET, VA와 PA를 매핑한 정보를 페이지 테이블에 추가해야 한다. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: 페이지 테이블 항목을 삽입하여 페이지의 가상 주소(VA)를 프레임의 물리 주소(PA)에 매핑합니다.*/
	struct thread *current = thread_current();
	pml4_set_page(current->pml4, page->va, frame->kva, page->writable);
	bool success = swap_in (page, frame->kva);
	// printf("success? %d\n", success);
	return  success;// uninit_initialize
}
/* (수정, 2)Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

/* (수정, 3)Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->va < b->va;
}

/* (수정, 1) Initialize new supplemental page table 
 * 새로운 SPT를 초기화. - 보조 페이지 테이블을 어떤 자료구조로 구현할지 선택해야함. -> 해쉬테이블
 * userprog/process.c의 initd 함수로 새로운 프로세스가 시작하거나, 
 * process.c의 __do_fork로 자식 프로세스가 생성될 때 위의 함수가 호출된다.
 */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/* hash_init()으로 해시테이블 초기화 */
	// printf("process.c:287\n");
	hash_init(&spt->spt_hash, page_hash, page_less, NULL);
	// printf("spt_init->hash_init | vm.c:290\n");
	/* 인자로 해시 테이블(초기화할 테이블)과 vm_hash_func(해시값을 구해주는 함수의 포인터)과 
	vm_less_func(해시 element들의 크기를 비교해주는 함수의 포인터) 사용 */
}

/* (수정,14) Copy supplemental page table from src to dst
	src에서 dst로 보조 페이지 테이블을 복사합니다. 
	이것은 자식이 부모의 실행 contxt를 상속할 필요가 있을 때 사용된다. 
	src의 spt를 반복하며 dst의 spt의 엔트리의 정확한 복사본을 만드세요
	당신은 초기화되지 않은 페이지를 할당하고 그것들을 요청할 필요가 있다 */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	struct hash_iterator i;
	hash_first(&i, &src->spt_hash);
	while(hash_next(&i))
	{
		struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type type = p->operations->type;
		void *upage = p->va;
		bool writable = p->writable;
	
		if (type == VM_UNINIT) {
			vm_alloc_page_with_initializer(p->uninit.type, p->va, 
										1, p->uninit.init, p->uninit.aux);
			vm_claim_page(p->va);
			struct page *child_page = spt_find_page(&thread_current()->spt, p->va);
		} else if (type == VM_ANON) {
			vm_alloc_page(VM_ANON, p->va, 1);
			struct page *child_p = spt_find_page(&thread_current()->spt, p->va);

			vm_do_claim_page(child_p);
			memcpy(child_p->frame->kva, p->frame->kva, PGSIZE);

		} else if (type = VM_FILE) {
			// struct file_info *tmp = (struct file_info *)p->file.aux;

			// vm_alloc_page(VM_FILE, p->va, 1);

			// struct page *child_p = (struct page *)spt_find_page(&thread_current()->spt, p->va);

			// vm_do_claim_page(child_p);
			// 주석처리 #1 process.h file_page *aux 관련 이슈
			// struct file_page *file_page = &child_p -> file;
		
			// file_page->aux = tmp;
			// memcpy(child_p->frame->kva, p->frame->kva, PGSIZE);
			struct lazy_load_arg *file_aux = malloc(sizeof(struct lazy_load_arg));
			file_aux->file = p->file.file;
			file_aux->ofs = p->file.ofs;
			file_aux->read_bytes = p->file.read_bytes;
			file_aux->zero_bytes = p->file.zero_bytes;
			if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, file_aux))
				return false;
			struct page *file_page = spt_find_page(dst, upage);
			file_backed_initializer(file_page, type, NULL);
			file_page->frame = p->frame;
			pml4_set_page(thread_current()->pml4, file_page->va, p->frame->kva, p->writable);
		}
	// } else { = 예린씨 코드
	// 	vm_alloc_page(p->operations->type, p->va, 1);
	// 	struct page *child_page = spt_find_page(&thread_current()->spt, p->va);
	// 	vm_claim_page(p->va);
	// 	memcpy(child_page->frame->kva, p->frame->kva, PGSIZE);
	// }
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->spt_hash, supplemental_destroy_entry);
}

void
supplemental_destroy_entry(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(page);
}
