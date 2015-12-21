#include <xtf/test.h>
#include <xtf/report.h>
#include <xtf/console.h>

#include <xtf/traps.h>
#include <xtf/exlog.h>

#include <arch/x86/processor.h>
#include <arch/x86/segment.h>

static void test_int3_breakpoint(void)
{
    printk("Test: int3 breakpoint\n");

    /*
     * Check that a breakpoint returns normally from the trap handler.
     */
    asm volatile ("int3");
}

static void test_extable(void)
{
    printk("Test: Exception Table\n");

    /*
     * Check that control flow is successfully redirected with a ud2a
     * instruction and appropriate extable entry.
     */
    asm volatile ("1: ud2a; 2:"
                  _ASM_EXTABLE(1b, 2b));
}

static bool check_nr_entries(unsigned int nr)
{
    unsigned int entries = xtf_exlog_entries();

    if ( entries != nr )
    {
        xtf_failure("Fail: expected %u entries, got %u\n",
                    nr, entries);
        return false;
    }

    return true;
}

static bool check_exlog_entry(unsigned int entry, unsigned int cs,
                              unsigned long ip, unsigned int ev,
                              unsigned int ec)
{
    exlog_entry_t *e = xtf_exlog_entry(entry);

    /* Check whether the log entry is available. */
    if ( !e )
    {
        xtf_failure("Fail: unable to retrieve log entry %u\n", entry);
        return false;
    }

    /* Check whether the log entry is correct. */
    if ( (e->ip != ip) || (e->cs != cs) || (e->ec != ec) || (e->ev != ev) )
    {
        xtf_failure("Fail: exlog entry:\n"
                    "  Expected: %04x:%p, ec %04x, vec %u\n"
                    "       Got: %04x:%p, ec %04x, vec %u\n",
                    cs, _p(ip), ec, ev, e->cs, _p(e->ip), e->ec, e->ev);
        return false;
    }

    return true;
}

static void test_exlog(void)
{
    extern unsigned long label_test_exlog_int3[], label_test_exlog_ud2a[];

    printk("Test: Exception Logging\n");

    xtf_exlog_start();

    /* Check that no entries have been logged thus far. */
    if ( !check_nr_entries(0) )
        goto out;

    asm volatile ("int3; label_test_exlog_int3:");

    /* Check that one entry has now been logged. */
    if ( !check_nr_entries(1) ||
         !check_exlog_entry(0, __KERN_CS,
                            (unsigned long)&label_test_exlog_int3,
                            X86_EXC_BP, 0) )
        goto out;

    asm volatile ("label_test_exlog_ud2a: ud2a; 1:"
                  _ASM_EXTABLE(label_test_exlog_ud2a, 1b));

    /* Check that two entries have now been logged. */
    if ( !check_nr_entries(2) ||
         !check_exlog_entry(1, __KERN_CS,
                            (unsigned long)&label_test_exlog_ud2a,
                            X86_EXC_UD, 0) )
        goto out;

    xtf_exlog_reset();

    /* Check that no entries now exist. */
    if ( !check_nr_entries(0) )
        goto out;

    asm volatile ("int3");

    /* Check that one entry now exists. */
    if ( !check_nr_entries(1) )
        goto out;

    xtf_exlog_stop();

    /* Check that one entry still exists. */
    if ( !check_nr_entries(1) )
        goto out;

    asm volatile ("int3");

    /* Check that the previous breakpoint wasn't logged. */
    if ( !check_nr_entries(1) )
        goto out;

 out:
    xtf_exlog_reset();
    xtf_exlog_stop();
}

void test_main(void)
{
    printk("XTF Selftests\n");

    test_int3_breakpoint();
    test_extable();
    test_exlog();

    xtf_success();
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
