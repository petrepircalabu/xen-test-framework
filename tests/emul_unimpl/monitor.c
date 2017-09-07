/**
 * @file tests/emul_unimpl/monitor.c
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct emul_unimpl_monitor
{
    xtf_monitor_t mon;
    domid_t domain_id;
    uint64_t address;
    uint16_t altp2m_view_id;
    void *map;
    unsigned long gfn;
    unsigned char instr[8];
    unsigned long instr_size;
} emul_unimpl_monitor_t;

const char monitor_test_help[] = \
    "Usage: test-monitor-emul_unimpl [options] <domid>\n"
    "\t -a <address>: the address where an invalid instruction will be injected\n"
    ;

static int emul_unimpl_setup(int argc, char *argv[]);
static int emul_unimpl_init();
static int emul_unimpl_run();
static int emul_unimpl_cleanup();

static emul_unimpl_monitor_t monitor_instance =
{
    .mon =
    {
        .setup      = emul_unimpl_setup,
        .init       = emul_unimpl_init,
        .run        = emul_unimpl_run,
        .cleanup    = emul_unimpl_cleanup,
    }
};

static int emul_unimpl_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
static int emul_unimpl_singlestep(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
static int emul_unimpl_emul_unimpl(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);

static xtf_evtchn_t evtchn_instance =
{
    .ops = 
    {
        .mem_access_handler     = emul_unimpl_mem_access,
        .singlestep_handler     = emul_unimpl_singlestep,
        .emul_unimpl_handler    = emul_unimpl_emul_unimpl,
    }
};

static int emul_unimpl_setup(int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        {"help",    no_argument,    0,  'h'},
        {"address", required_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;

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

static int emul_unimpl_init()
{
    int rc = 0;
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;

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

    rc = xc_monitor_emul_unimplemented(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d emulation unimplemented with vm_event\n", rc);
        return rc;
    }

    rc = xc_altp2m_set_domain_state(xtf_xch, pmon->domain_id, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d enabling altp2m on domain!\n", rc);
        return rc;
    }

    rc = xc_altp2m_create_view(xtf_xch, pmon->domain_id, XENMEM_access_rwx,
            &pmon->altp2m_view_id );
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d creating altp2m view!\n", rc);
        return rc;
    }

    pmon->gfn = xc_translate_foreign_address(xtf_xch, pmon->domain_id, 0, pmon->address);

    pmon->map = xc_map_foreign_range(xtf_xch, pmon->domain_id, 4096,
            PROT_READ | PROT_WRITE , pmon->gfn);
    if ( !pmon->map )
    {
        XTF_MON_ERROR("Failed to map page.\n");
        return -1;
    }

    rc = xc_altp2m_set_mem_access(xtf_xch, pmon->domain_id, pmon->altp2m_view_id,
            pmon->gfn, XENMEM_access_rw);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting altp2m memory access!\n", rc);
        return rc;
    }

    rc = xc_altp2m_switch_to_view(xtf_xch, pmon->domain_id, pmon->altp2m_view_id );
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d switching to altp2m view!\n", rc);
        return rc;
    }

    rc = xc_monitor_singlestep(xtf_xch, pmon->domain_id, 1 );
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d failed to enable singlestep monitoring!\n", rc);
        return rc;
    }

    return 0;
}

static int emul_unimpl_run()
{
    int rc;
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    rc = xc_domain_unpause(xtf_xch, pmon->domain_id);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error unpausing domain.\n");
        return rc;
    }

    return xtf_evtchn_loop(pmon->domain_id);
}

static int emul_unimpl_cleanup()
{
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    if (pmon->map)
    {
        munmap(pmon->map, 4096);
    }

    xc_altp2m_switch_to_view(xtf_xch, pmon->domain_id, 0 );

    xc_altp2m_destroy_view(xtf_xch, pmon->domain_id, pmon->altp2m_view_id);

    xc_altp2m_set_domain_state(xtf_xch, pmon->domain_id, 0);

    xc_monitor_singlestep(xtf_xch, pmon->domain_id, 0);

    xtf_evtchn_cleanup(pmon->domain_id);

    return 0;
}


static int emul_unimpl_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;
    volatile unsigned char *p;
    int i;

    if (!pmon)
        return -EINVAL;

    rsp->flags |= VM_EVENT_FLAG_EMULATE | VM_EVENT_FLAG_TOGGLE_SINGLESTEP;

    p = (volatile unsigned char*)(pmon->map + req->u.mem_access.offset);

    pmon->instr[0] = *(p + 0);
    pmon->instr[1] = *(p + 1);
    pmon->instr[2] = *(p + 2);
    pmon->instr[3] = *(p + 3);
    pmon->instr[4] = *(p + 4);
    *(p + 0) = 0xD9;
    *(p + 1) = 0x20;
    *(p + 2) = 0x00;
    *(p + 3) = 0x00;
    *(p + 4) = 0x00;
    pmon->instr_size = 5;

    rsp->u.mem_access = req->u.mem_access;
    return 0;
}

static int emul_unimpl_singlestep(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;

    if (!pmon)
        return -EINVAL;

    rsp->flags |= VM_EVENT_FLAG_ALTERNATE_P2M | VM_EVENT_FLAG_TOGGLE_SINGLESTEP;
    rsp->altp2m_idx = pmon->altp2m_view_id;

    /* Restore the execute rights on the test page. */
    xc_altp2m_set_mem_access(xtf_xch, pmon->domain_id, pmon->altp2m_view_id,
        pmon->gfn, XENMEM_access_rwx);
    return 0;
}

static int emul_unimpl_emul_unimpl(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    emul_unimpl_monitor_t *pmon = (emul_unimpl_monitor_t *)monitor;
    volatile unsigned char *p;
    int i;

    if (!pmon)
        return -EINVAL;

    p = (volatile unsigned char *)(pmon->map + req->u.mem_access.offset);

    for (i = 0; i < pmon->instr_size; i++)
        *(p + i) = pmon->instr[i];

    rsp->flags |= VM_EVENT_FLAG_ALTERNATE_P2M ;
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
