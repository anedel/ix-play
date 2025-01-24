
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util-input.h"


/*
 * Make all mmap()-related areas in this demonstration
 * multiples of this unit; should be at least 32 KBytes
 * (even if the typical page size is 4 or 8 KBytes) --- or, better,
 * 64 KBytes: easier to read and compute with when shown in hexadecimal.
 */
static const size_t  Mem_Map_Unit = 64 * 1024;

static const char *const Prompt_Str = "Press Enter to continue";

/*
 * If this variable is zero ('want_offset_adjust = 0' below)
 * the mappings will not be merged in the last steps of this program.
 *
 * Apparently the kernel does not see '/dev/zero' as a special case
 * (does not recognize that all zero pages are equivalent).
 *
 * If we map again a memory subrange but we do not not specify
 * the corresponding file offset (saved as 'm_fd_offset' below) as it was
 * in the initial mapping for the pages in that subrange, the kernel
 * will not see the resulting combination of sub-mappings as contiguous
 * so our test mapping will not merge back.
 *
 * [AN, 2014-10-02]  My current understanding can be summarized as:
 * Merging can occur only when the neighboring or overlapping
 * memory mappings have same protection flags and are contiguous in _both_
 *   - the virtual address space _and_
 *   - the source space = offsets in the mapped file, shared memory object,
 *        or typed memory object (whatever that means, I know no examples).
 */
static int  want_offset_adjust = 1;


typedef struct {
    void   *m_base_addr;
    size_t  m_len;
    size_t  m_unit_offset;  /* number of units from the base of the main range */
    off_t   m_fd_offset;  /* in bytes */
} mem_range;


static void
show_range_ (const mem_range *mr, FILE *out_stream)
{
    void *const end_addr = (char *) mr->m_base_addr + mr->m_len;

    fprintf(out_stream,
            "\n  Base address %p (at +%zu units, fd offset %llu), length 0x%zx = %zu bytes;"
            "\n   end address %p.\n",
            mr->m_base_addr, mr->m_unit_offset, (unsigned long long) mr->m_fd_offset,
            mr->m_len, mr->m_len,
            end_addr);
}

static void
fill_subrange_ (mem_range *subrange, const mem_range *base_range,
                size_t offset_in_units, size_t len_in_units)
{
    const size_t  subrange_offset = offset_in_units * Mem_Map_Unit;

    subrange->m_base_addr = (char *) base_range->m_base_addr + subrange_offset;

    subrange->m_len = len_in_units * Mem_Map_Unit;

    subrange->m_unit_offset = offset_in_units;

    subrange->m_fd_offset = (base_range->m_fd_offset +
                             subrange_offset * want_offset_adjust);
}


static int  mapped_fd = -1;


static void
mmap_fixed_range_ (const mem_range *mr, int prot)
{
    void *const address = mmap(mr->m_base_addr, mr->m_len, prot,
                               MAP_PRIVATE | MAP_FIXED,
                               mapped_fd, mr->m_fd_offset);
    if (MAP_FAILED == address) {
        perror("mmap(... MAP_FIXED ...)");
        exit(4);
    }

    if (mr->m_base_addr != address) {
        fprintf(stderr, "mmap(... MAP_FIXED ...) returned %p != requested address %p\n",
                address, mr->m_base_addr);
        exit(5);
    }

    printf("Done: mmap");
    show_range_(mr, stdout);
}

static void
munmap_range_ (const mem_range *mr)
{
    const int  res = munmap(mr->m_base_addr, mr->m_len);

    if (res != 0) {
        perror("munmap");
        exit(8);
    }

    printf("Done: munmap");
    show_range_(mr, stdout);
}

static void
mprotect_range_ (const mem_range *mr, int prot)
{
    const int  res = mprotect(mr->m_base_addr, mr->m_len, prot);

    if (res != 0) {
        perror("mprotect");
        exit(9);
    }

    printf("Done: mprotect");
    show_range_(mr, stdout);
}


int
main (void)
{
    mem_range  mr_test_area;  /* our playground */
    mem_range  mr_subarea1;  /* part of 'mr_test_area' */
    mem_range  mr_subarea2;  /* other part of 'mr_test_area' */

    mapped_fd = open("/dev/zero", O_RDWR);

    if (mapped_fd < 0) {
        perror("open /dev/zero");
        exit(11);
    }

    printf("Pid = %ld\n", (long) getpid());

    mr_test_area.m_unit_offset = 0;  /* zero by definition for the main area */
    mr_test_area.m_fd_offset = 0;  /* could be a different offset if we want */

    mr_test_area.m_len = 5 * Mem_Map_Unit;

    mr_test_area.m_base_addr = mmap(0, mr_test_area.m_len,
                                    PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE,
                                    mapped_fd, mr_test_area.m_fd_offset);
    if (MAP_FAILED == mr_test_area.m_base_addr) {
        perror("mmap(0, 5 units, ...)");
        exit(12);
    }

    printf("\nOur test area = ");
    show_range_(&mr_test_area, stdout);

    fill_subrange_(&mr_subarea1, &mr_test_area, 1, 2);
    fill_subrange_(&mr_subarea2, &mr_test_area, 2, 2);

    printf("Sub-area 1 = ");
    show_range_(&mr_subarea1, stdout);
    printf("Sub-area 2 = ");
    show_range_(&mr_subarea2, stdout);

    printf("\nNext: changing protection to read-only on sub-area 1...\n");
    pause_prompt(STDIN_FILENO, stdout, Prompt_Str);

    mprotect_range_(&mr_subarea1, PROT_READ);

    printf("\nNext: unmapping sub-area 2...\n");
    pause_prompt(STDIN_FILENO, stdout, Prompt_Str);

    munmap_range_(&mr_subarea2);

    printf("\nNext: mapping again sub-area 1 (as writable)...\n");
    pause_prompt(STDIN_FILENO, stdout, Prompt_Str);

    mmap_fixed_range_(&mr_subarea1, PROT_READ | PROT_WRITE);

    printf("\nNext: mapping again sub-area 2 (as writable)...\n");
    pause_prompt(STDIN_FILENO, stdout, Prompt_Str);

    mmap_fixed_range_(&mr_subarea2, PROT_READ | PROT_WRITE);

    printf("\nEnd.\n");

    pause_prompt(STDIN_FILENO, stdout, "Press Enter to finish (exit program).");

    return 0;
}
