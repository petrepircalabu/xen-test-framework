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
#include <xtf/hypercall.h>

const char test_title[] = "HVMOP altp2m set_access_multi";

static uint8_t test_page1[PAGE_SIZE] __page_aligned_bss;
static uint8_t test_page2[PAGE_SIZE] __page_aligned_bss;
static uint8_t test_page3[PAGE_SIZE] __page_aligned_bss;

void test_main(void)
{
    int rc;
    xenmem_access_t default_access = XENMEM_access_r;
    uint16_t view_id = 0;

    rc = hvm_altp2m_set_domain_state(true);
    if ( rc < 0 )
        return xtf_error("Error %d enabling altp2m on domain!\n", rc);

    rc = hvm_altp2m_create_view(default_access, &view_id);
    if ( rc < 0 )
        return xtf_error("Error %d creating altp2m view!\n", rc);

    test_page1[0] = 1;
    test_page2[0] = 1;
    test_page3[0] = 1;

    rc = hvm_altp2m_set_domain_state(false);
    if ( rc < 0 )
        return xtf_error("Error %d disabling altp2m view!\n", rc);

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

