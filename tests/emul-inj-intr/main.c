/**
 * @file tests/emul-inj-intr/main.c
 *
 */
#include <xtf.h>

const char test_title[] = " Emulation inject interrupt";

void test_main(void)
{
    asm("hlt");
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
