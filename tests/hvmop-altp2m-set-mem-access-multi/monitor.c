/**
 * @file tests/hvmop-altp2m-set-mem-access-multi/monitor.c
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct hvmop_altp2m_monitor
{
    xtf_monitor_t mon;
    domid_t domain_id;
} hvmop_altp2m_monitor_t;

const char monitor_test_help[] = \
    "Usage: test-monitor-hvmop-altp2m_set_mem_access_multi <domid>\n";

static int hvmop_altp2m_setup(int argc, char *argv[]);
static int hvmop_altp2m_init();
static int hvmop_altp2m_run();
static int hvmop_altp2m_cleanup();

static hvmop_altp2m_monitor_t monitor_instance = 
{
    .mon =
    {
        .setup      = hvmop_altp2m_setup,
        .init       = hvmop_altp2m_init,
        .run        = hvmop_altp2m_run,
        .cleanup    = hvmop_altp2m_cleanup,
    }
};

static int hvmop_altp2m_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);

static xtf_evtchn_t evtchn_instance =
{
    .ops = 
    {
        .mem_access_handler     = hvmop_altp2m_mem_access,
    }
};

static int hvmop_altp2m_setup(int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        {"help",    no_argument,    0,  'h'},
        {"address", required_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    hvmop_altp2m_monitor_t *pmon = (hvmop_altp2m_monitor_t *)monitor;
    printf("Setup\n");

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
                break;
            default:
                XTF_MON_ERROR("%s: Invalid option %s\n", argv[0], optarg);
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

static int hvmop_altp2m_init()
{
    int rc = 0;
    hvmop_altp2m_monitor_t *pmon = (hvmop_altp2m_monitor_t *)monitor;

    printf("Init\n");
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

    return rc;
}

static int hvmop_altp2m_run()
{
    int rc;
    hvmop_altp2m_monitor_t *pmon = (hvmop_altp2m_monitor_t *)monitor;

    printf("Run\n");

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

static int hvmop_altp2m_cleanup()
{
    hvmop_altp2m_monitor_t *pmon = (hvmop_altp2m_monitor_t *)monitor;

    if ( !pmon )
        return -EINVAL;

    xtf_evtchn_cleanup(pmon->domain_id);
    return 0;
}

static int hvmop_altp2m_mem_access(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp)
{
    printf("hvmop_altp2m_mem_access\n");
    return 0;
}

XTF_MONITOR(monitor_instance);
