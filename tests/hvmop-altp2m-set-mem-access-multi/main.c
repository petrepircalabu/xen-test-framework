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

/*
xenmem_access_t default_access = XENMEM_access_rwx;
xenmem_access_t access[3] = {
    XENMEM_access_rw,
    XENMEM_access_r,
    XENMEM_access_rw,
};
uint16_t view_id = 0;
*/
uint64_t pages[3];

/** Print expected information in the case of an unexpected exception. */
bool unhandled_exception(struct cpu_regs *regs)
{
    /*hvm_altp2m_set_mem_access(view_id, pages[1], XENMEM_access_rw);*/
    return false;
}

void test_main(void)
{
#if 0
    int rc;

    rc = hvm_altp2m_set_domain_state(true);
    if ( rc < 0 )
        return xtf_error("Error %d enabling altp2m on domain!\n", rc);
#endif

    pages[0] = virt_to_gfn(test_page1);
    pages[1] = virt_to_gfn(test_page2);
    pages[2] = virt_to_gfn(test_page3);

    printk("gfn1 = %ld gfn2 = %ld gfn3 = %ld\n", pages[0], pages[1], pages[2]);

#if 0
    /* Setup.  Hook unhandled exceptions for debugging purposes. */
    xtf_unhandled_exception_hook = unhandled_exception;

    rc = hvm_altp2m_create_view(default_access, &view_id);
    if ( rc < 0 )
        return xtf_error("Error %d creating altp2m view!\n", rc);

    rc = hvm_altp2m_set_mem_access_multi(view_id, access, pages, 3);
    if ( rc < 0 )
        return xtf_error("Error %d setting altp2m access!\n", rc);

    rc = hvm_altp2m_vcpu_enable_notify(0, pages[1]);
    if ( rc < 0 )
        return xtf_error("Error %d enabling vcpu notify!\n", rc);

    rc = hvm_altp2m_switch_to_view(view_id);
    if ( rc < 0 )
        return xtf_error("Error %d switing altp2m view!\n", rc);
#endif

    test_page1[0] = 1;
    test_page2[0] = 1;
    test_page3[0] = 1;
#if 0
    rc = hvm_altp2m_set_domain_state(false);
    if ( rc < 0 )
        return xtf_error("Error %d disabling altp2m view!\n", rc);
#endif
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

