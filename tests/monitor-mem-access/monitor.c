/**
 * @file tests/emul-unimpl/monitor.c
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct mem_access_monitor
{
    xtf_monitor_t mon;
    uint64_t address;
    unsigned long gfn;
} mem_access_monitor_t;

static const char mem_access_test_help[] = \
    "Usage: test-monitor-mem_access [options] <domid>\n"
    "\t -a <address>: the start address of the execution protected page\n"
    ;

static int mem_access_setup(int argc, char *argv[]);
static int mem_access_init();
static int mem_access_get_result();

static mem_access_monitor_t monitor_instance =
{
    .mon =
    {
        .help_message = mem_access_test_help,
        .ops =
        {
            .setup      = mem_access_setup,
            .init       = mem_access_init,
            .get_result = mem_access_get_result,
        }
    }
};

static int test_mem_access(vm_event_request_t *req, vm_event_response_t *rsp);

static int mem_access_setup(int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        {"help",    no_argument,    0,  'h'},
        {"address", required_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    mem_access_monitor_t *pmon = (mem_access_monitor_t *)monitor;

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

    monitor->domain_id = atoi(argv[optind]);

    if ( monitor->domain_id <= 0 )
    {
        XTF_MON_ERROR("%s: Invalid domain id\n", argv[0]);
        return -EINVAL;
    }

    monitor->handlers[VM_EVENT_REASON_MEM_ACCESS] = test_mem_access;

    return 0;
}

static int mem_access_init()
{
    int rc = 0;
    mem_access_monitor_t *pmon = (mem_access_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    rc = xc_domain_set_access_required(monitor->xch, monitor->domain_id, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting mem_access listener required\n", rc);
        return rc;
    }

    pmon->gfn = xc_translate_foreign_address(monitor->xch, monitor->domain_id,
                                             0, pmon->address);
    if ( pmon->gfn == 0 )
    {
        XTF_MON_ERROR("Failed to translate the foreign address!\n");
        return -errno;
    }

    rc = xc_set_mem_access(monitor->xch, monitor->domain_id, XENMEM_access_rw,
                           pmon->gfn, 1);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Error %d setting memory access!\n", rc);
        return rc;
    }

    return 0;
}

static int mem_access_get_result()
{
    return XTF_MON_STATUS_SUCCESS;
}

static int test_mem_access(vm_event_request_t *req, vm_event_response_t *rsp)
{
    return xc_set_mem_access(monitor->xch, monitor->domain_id,
                             XENMEM_access_rwx, req->u.mem_access.gfn, 1);
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
