/**
 * @file tests/monitor-mem-access/main.c
 * @ref test-monitor-mem-access
 *
 * @page test-monitor-mem-access monitor-mem-access
 */

#include <xtf.h>

const char test_title[] = "Test monitor mem-access event";

static int count;

static void __attribute__((section(".text.protected"))) __attribute__((noinline)) test_fn(void)
{
    count++;
}

void test_main(void)
{
    test_fn();
    
    if ( count )
        xtf_success(NULL);
    else
        xtf_failure(NULL);
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
