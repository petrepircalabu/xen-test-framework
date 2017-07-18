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
