/**
 * @file tests/emul_unhandleable/monitor.c
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct emul_unhandleable_monitor
{
    xtf_monitor_t mon;
    domid_t domain_id;
    uint64_t address;
    uint16_t altp2m_view_id;
    void *map;
    unsigned long instr;
} emul_unhandleable_monitor_t;

const char monitor_test_help[] = \
    "Usage: test-monitor-emul_unhandleable [options] <domid>\n"
    "\t -a <address>: the address where an invalid instruction will be injected\n"
    ;

static int emul_unhandleable_setup(int argc, char *argv[]);
static int emul_unhandleable_init();
static int emul_unhandleable_run();
static int emul_unhandleable_cleanup();

static emul_unhandleable_monitor_t monitor_instance =
{
    .mon =
    {
        .setup      = emul_unhandleable_setup,
        .init       = emul_unhandleable_init,
        .run        = emul_unhandleable_run,
        .cleanup    = emul_unhandleable_cleanup,
    }
};

static int emul_unhandleable_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
static int emul_unhandleable_singlestep(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
static int emul_unhandleable_emul_unimpl(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);

static xtf_evtchn_ops_t evtchn_handlers =
{
    .mem_access_handler     = emul_unhandleable_mem_access,
    .singlestep_handler     = emul_unhandleable_singlestep,
    .emul_unimpl_handler    = emul_unhandleable_emul_unimpl,
};

static xtf_evtchn_t evtchn_instance;

static int emul_unhandleable_setup(int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        {"help",    no_argument,    0,  'h'},
        {"address", no_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;

    /* assert(pmon) */

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
                pmon->address = atol(optarg);
                break;
            default:
                fprintf(stderr, "%s: Invalid option %s\n", argv[0], optarg);
                return -EINVAL;
        }

        if ( !pmon->address )
        {
            fprintf(stderr, "%s: Please specify a valid instruction injection address\n", argv[0]);
            return -EINVAL;
        }

        if ( optind != argc - 1 )
        {
            fprintf(stderr, "%s: Please specify the domain id\n", argv[0]);
            return -EINVAL;
        }
    }

    pmon->domain_id = atoi(argv[optind]);

    printf("Domain_id = %d\n", pmon->domain_id);

    /* assert(evtchn_instance.domain_id) */
    add_evtchn(&evtchn_instance, pmon->domain_id);

    return 0;
}

static int emul_unhandleable_init()
{
    int rc = 0;
    unsigned long gfn = 0x105;
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    rc = xtf_evtchn_init(pmon->domain_id);
    if ( rc < 0 )
        return rc;

    rc = xc_domain_set_access_required(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d setting mem_access listener required\n", rc);
        return rc;
    }

    rc = xc_monitor_emul_unimplemented(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d emulation unimplemented with vm_event\n", rc);
        return rc;
    }


    rc = xc_altp2m_set_domain_state(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d enabling altp2m on domain!\n", rc);
        return rc;
    }

    rc = xc_altp2m_create_view(xtf_xch, pmon->domain_id, XENMEM_access_rwx,
            &pmon->altp2m_view_id );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d creating altp2m view!\n", rc);
        return rc;
    }

    rc = xc_altp2m_set_mem_access(xtf_xch, pmon->domain_id, pmon->altp2m_view_id,
            0x105, XENMEM_access_rw);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d setting altp2m memory access!\n", rc);
        return rc;
    }


    gfn = xc_translate_foreign_address(xtf_xch, pmon->domain_id, 0, 0x105000);

    pmon->map = xc_map_foreign_range(xtf_xch, pmon->domain_id, 4096,
            PROT_READ | PROT_WRITE , 0);
    if ( !pmon->map )
    {
        fprintf(stderr, "Failed to map page.\n");
        return -1;
    }

    rc = xc_altp2m_switch_to_view(xtf_xch, pmon->domain_id, pmon->altp2m_view_id );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d switching to altp2m view!\n", rc);
        return rc;
    }

    rc = xc_monitor_singlestep(xtf_xch, pmon->domain_id, 1 );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d failed to enable singlestep monitoring!\n", rc);
        return rc;
    }

    return 0;
}

static int emul_unhandleable_run()
{
    int rc;
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    return xtf_evtchn_loop(pmon->domain_id);
}

static int emul_unhandleable_cleanup()
{
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    xtf_evtchn_cleanup(pmon->domain_id);

    xc_altp2m_switch_to_view(xtf_xch, pmon->domain_id, 0 );

    xc_altp2m_destroy_view(xtf_xch, pmon->domain_id, pmon->altp2m_view_id);

    xc_altp2m_set_domain_state(xtf_xch, pmon->domain_id, 0);

    xc_monitor_singlestep(xtf_xch, pmon->domain_id, 0);

    if (pmon->map)
    {
        munmap(pmon->map, 4096);
    }

    return 0;
}

static int emul_unhandleable_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;
    volatile unsigned long *p;

    if (!pmon)
        return -EINVAL;

    rsp->flags |= VM_EVENT_FLAG_EMULATE;

    p = (volatile unsigned long *)pmon->map;
    pmon->instr = *p;
    *p = 0xDEADBABE;

    rsp->u.mem_access = req->u.mem_access;

    return 0;
}

static int emul_unhandleable_singlestep(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;

    if (!pmon)
        return -EINVAL;

    rsp->flags |= VM_EVENT_FLAG_ALTERNATE_P2M | VM_EVENT_FLAG_TOGGLE_SINGLESTEP;
    rsp->altp2m_idx = pmon->altp2m_view_id;

    return 0;
}

static int emul_unhandleable_emul_unimpl(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unhandleable_monitor_t *pmon = (emul_unhandleable_monitor_t *)monitor;
    volatile unsigned long *p;

    if (!pmon)
        return -EINVAL;

    p = (volatile unsigned long *)pmon->map;
    *p = pmon->instr;

    rsp->flags |= (VM_EVENT_FLAG_ALTERNATE_P2M | VM_EVENT_FLAG_TOGGLE_SINGLESTEP);
    rsp->altp2m_idx = 0;

    return 0;
}

XTF_MONITOR(monitor_instance);

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
