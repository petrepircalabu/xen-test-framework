/**
 * @file tests/emul-inj-intr/main.c
 *
 */
#include <xtf.h>

const char test_title[] = " Emulation inject interrupt";

void test_idte_handler(void);
asm ("test_idte_handler:;"
     "rex64 iret");

static void __attribute__((section(".text.secondary"))) __attribute__ ((noinline)) test_fn(void)
{
    __asm__ __volatile__ ("hlt ");
}

void test_main(void)
{
    __asm__ __volatile__ ("cli");

    int rc = apic_init(APIC_MODE_XAPIC);
    if ( rc )
        return xtf_error("Error: Unable to set up xapic mode: %d\n", rc);

    struct xtf_idte idte = {
        .addr = _u(test_idte_handler),
        .cs = __KERN_CS,
    };

    rc = xtf_set_idte(X86_VEC_AVAIL, &idte);
    if ( rc )
        return xtf_failure("Fail: xtf_set_idte() returned %d\n", rc);

    apic_write(APIC_LVTT, X86_VEC_AVAIL);
    apic_write(APIC_TDCR, 0xA);
    apic_write(APIC_TMICT, 0x0000FFFF);

    __asm__ __volatile__ ("sti ");
    test_fn();

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
