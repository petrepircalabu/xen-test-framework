/**
 * @file tests/emul_unimpl/main.c
 * @ref test-emul_unimpl
 *
 * @page test-emul_unimpl emul_unimpl
 *
 * @todo Docs for test-emul_unimpl
 *
 * @see tests/emul_unimpl/main.c
 */
#include <xtf.h>

const char test_title[] = "Test emul_unimpl";

static char fpu_env[128];

static void __attribute__((section(".text.secondary"))) __attribute__ ((noinline)) test_fn(void)
{
    __asm__ __volatile__("fstenv %0"
                         : "=m" (fpu_env)
                         :
                         : "memory");
    __asm__ __volatile__("fwait");
}

void test_main(void)
{
    int i;

    __asm__ __volatile__ ("pushf");
    __asm__ __volatile__ ("cli");
    test_fn();
    __asm__ __volatile__ ("popf");

    for ( i = 0; i < 14 ; i++ )
    {
        if ( fpu_env[i] != 0 )
            break;
    }

    if ( i == 14 )
    {
        xtf_error(NULL);
    }
    else
    {
        xtf_success(NULL);
    }
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
