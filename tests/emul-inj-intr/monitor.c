/**
 * @file tests/emul-inj-intr/monitor.c
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct emul_inj_intr_monitor
{
    xtf_monitor_t mon;
    domid_t domain_id;
    uint64_t address;
    unsigned long gfn;
} emul_inj_intr_monitor_t;

const char monitor_test_help[] = \
    "Usage: test-monitor-emul-inj-intr [options] <domid>\n"
    "\t -a <address>: the address where an invalid instruction will be injected\n"
    ;

static int emul_inj_intr_setup(int argc, char *argv[]);
static int emul_inj_intr_init();
static int emul_inj_intr_run();
static int emul_inj_intr_cleanup();
static int emul_inj_intr_get_result();

static emul_inj_intr_monitor_t monitor_instance =
{
    .mon =
    {
        .setup      = emul_inj_intr_setup,
        .init       = emul_inj_intr_init,
        .run        = emul_inj_intr_run,
        .cleanup    = emul_inj_intr_cleanup,
        .get_result = emul_inj_intr_get_result,
    }
};

static int emul_inj_intr_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);

static xtf_evtchn_t evtchn_instance =
{
    .ops =
    {
     .mem_access_handler = emul_inj_intr_mem_access,
    }
};

static int emul_inj_intr_setup(int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        {"help",    no_argument,    0,  'h'},
        {"address", required_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    if ( argc == 1 )
    {
        usage();
        return -EINVAL;
    }
    while ( 1 )
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "ha:", long_options, &option_index);
        if ( c == -1 ) break;

        switch ( c )
        {
            case 'h':
                usage();
                exit(0);
                break;
            case 'a':
                pmon->address = strtoul(optarg, NULL, 0);
                break;
            default:
                XTF_MON_ERROR("%s: Invalid option %s\n", argv[0], optarg);
                return -EINVAL;
        }

        if ( !pmon->address )
        {
            XTF_MON_ERROR("%s: Please specify a valid instruction injection address\n", argv[0]);
            return -EINVAL;
        }

        if ( optind != argc - 1 )
        {
            XTF_MON_ERROR("%s: Please specify the domain id\n", argv[0]);
            return -EINVAL;
        }
    }

    pmon->domain_id = atoi(argv[optind]);

    if ( pmon->domain_id <= 0 )
    {
        XTF_MON_ERROR("%s: Invalid domain id\n", argv[0]);
        return -EINVAL;
    }

    add_evtchn(&evtchn_instance, pmon->domain_id);

    return 0;
}

static int emul_inj_intr_init()
{
    int rc = 0;
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    rc = xtf_evtchn_init(pmon->domain_id);
    if ( rc < 0 )
        return rc;

    rc = xc_domain_set_access_required(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting mem_access listener required\n", rc);
        return rc;
    }

    pmon->gfn = xc_translate_foreign_address(xtf_xch, pmon->domain_id, 0, pmon->address);

    rc = xc_set_mem_access(xtf_xch, pmon->domain_id, XENMEM_access_rw, pmon->gfn, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting altp2m memory access!\n", rc);
        return rc;
    }

    return 0;
}

static int emul_inj_intr_run()
{
    int rc;
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    return xtf_evtchn_loop(pmon->domain_id);
}

static int emul_inj_intr_cleanup()
{
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    xtf_evtchn_cleanup(pmon->domain_id);

    return 0;
}

static int emul_inj_intr_get_result()
{
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;

    if ( !pmon )
        return XTF_MON_ERROR;

    return XTF_MON_SUCCESS;
}

static int emul_inj_intr_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_inj_intr_monitor_t *pmon = (emul_inj_intr_monitor_t *)monitor;
    volatile unsigned char *p;
    int rc = 0;

    if (!pmon)
        return -EINVAL;

    rsp->flags |= VM_EVENT_FLAG_EMULATE;
    rc = xc_set_mem_access(xtf_xch, pmon->domain_id, XENMEM_access_rwx, pmon->gfn, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting altp2m memory access!\n", rc);
        return rc;
    }

    return 0;
}


XTF_MONITOR(monitor_instance);

/**
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
