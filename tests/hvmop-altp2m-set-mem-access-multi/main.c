/**
 * @file tests/hvmop-altp2m-set-mem-access-multi/main.c
 * @ref hvmop-altp2m-set-mem-access-multi
 *
 * @page hvmop-altp2m-set-mem-access-multi
 *
 * @see tests/hvmop-altp2m-set-mem-access-multi/main.c
 */
#include <xtf.h>
#include <arch/mm.h>

const char test_title[] = "HVMOP altp2m set_access_multi";

static uint8_t test_page1[PAGE_SIZE] __page_aligned_bss;
static uint8_t test_page2[PAGE_SIZE] __page_aligned_bss;
static uint8_t test_page3[PAGE_SIZE] __page_aligned_bss;

void test_main(void)
{
    test_page1[0] = 1;
    test_page2[0] = 1;
    test_page3[0] = 1;
    xtf_success(NULL);
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

