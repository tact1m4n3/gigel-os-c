#include <string.h>
#include <debug.h>
#include <multiboot.h>
#include <spinlock.h>
#include <memory.h>
#include <cpu.h>

#define MASK_FLAGS(x) (x & ~0xFFF)

#define PRESENT(x) (x & PAGE_PRESENT)

#define PT(x) ((uint64_t*)MASK_FLAGS(x))
#define PT_ENTRIES 512

#define P4_IDX(x) (((x) >> 39) & 0x1FF)
#define P3_IDX(x) (((x) >> 30) & 0x1FF)
#define P2_IDX(x) (((x) >> 21) & 0x1FF)
#define P1_IDX(x) (((x) >> 12) & 0x1FF)

#define P4E (PT(p4)[P4_IDX(addr)])
#define P3E (PT(P4E)[P3_IDX(addr)])
#define P2E (PT(P3E)[P2_IDX(addr)])
#define P1E (PT(P2E)[P1_IDX(addr)])

extern uint64_t kernel_start, kernel_end;

uint64_t next_page;
uint64_t kernel_p4;
uint64_t boot_page_area[4][PT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

spinlock_t page_alloc_lock;

uint64_t alloc_page() {
    if (!next_page) {
        WARN("ran out of memory");
        return 0;
    }

    acquire_lock(&page_alloc_lock);
    uint64_t ret = next_page;
    next_page = *(uint64_t*)ret;
    release_lock(&page_alloc_lock);

    return ret;
}

uint64_t calloc_page() {
    uint64_t page = alloc_page();
    memset((void*)page, 0, PAGE_SIZE);
    return page;
}

void free_page(uint64_t page) {
    acquire_lock(&page_alloc_lock);
    *(uint64_t*)page = next_page;
    next_page = page;
    release_lock(&page_alloc_lock);
}

uint64_t get_page(uint64_t p4, uint64_t addr) {
    if (p4 && PRESENT(P4E) && PRESENT(P3E) && PRESENT(P2E))
        return P1E;
    return -1;
}

int map_page(uint64_t p4, uint64_t addr, uint64_t page, uint16_t flags) {
    if (!p4)
        return 0;

    if (!PRESENT(P4E))
        P4E = calloc_page() | flags | PAGE_PRESENT;
    if (!PRESENT(P3E))
        P3E = calloc_page() | flags | PAGE_PRESENT;
    if (!PRESENT(P2E))
        P2E = calloc_page() | flags | PAGE_PRESENT;
    P1E = page | flags | PAGE_PRESENT;

    return 1;
}

void memory_init() {
    kernel_p4 = read_cr3();

    INFO("initializing memory\n");
    uint64_t i = 0, type, start, end;
    while (multiboot_get_memory_area(i, &type, &start, &end)) {
        INFO("entry %x-%x %x\n", start, end, type);
        for (uint64_t j = start; j < end; j += PAGE_SIZE) {
            map_page(kernel_p4, j, j, PAGE_WRITE);
            if (type == MMAP_FREE)
                if (!(j >= (uint64_t)&kernel_start && j < (uint64_t)&kernel_end) && !multiboot_is_page_used(j))
                    free_page(j);
        }
        i++;
    }
}