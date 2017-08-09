/**
 * @file tests/emul_unhandleable/main.c
 * @ref test-emul_unhandleable
 *
 * @page test-emul_unhandleable emul_unhandleable
 *
 * @todo Docs for test-emul_unhandleable
 *
 * @see tests/emul_unhandleable/main.c
 */
#include <xtf.h>

const char test_title[] = "Test emul_unhandleable";

static int count;

static int __attribute__((section(".text.secondary"))) __attribute__ ((noinline)) test_fn(void)
{
    count++;
    return 0;
}

void test_main(void)
{
    count = 0;
    count++;
    if ( test_fn() || count != 2 )
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
