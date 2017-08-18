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

typedef struct emul_unhandleable_test
{
    uint64_t address;
    uint16_t altp2m_view_id;
    void *map;
} emul_unhandleable_test_t;

const char monitor_test_help[] = \
    "Usage: test-monitor-emul_unhandleable [options] <domid>\n"
    "\t -a <address>: the address where an invalid instruction will be injected\n"
    ;

static int emul_unhandleable_setup(xtf_monitor_t *pdata, int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        XTF_MONITOR_COMMON_OPTIONS_LONG,
        {"address", no_argument,    0,  'a'},
        {0, 0, 0, 0}
    };
    emul_unhandleable_test_t *priv = (emul_unhandleable_test_t *)pdata;

    if ( !priv )
        return -EINVAL;

    if ( argc == 1 )
    {
        usage();
        return -EINVAL;
    }

    while (1)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, XTF_MONITOR_COMMON_OPTIONS_SHORT "a:",
                long_options, &option_index);
        if ( c == -1 ) break;

        switch ( c )
        {
            XTF_MONITOR_COMMON_OPTIONS_HANDLER;
            case 'a':
                priv->address = atol(optarg);
                break;
            default:
                fprintf(stderr, "%s: Invalid option %s\n", argv[0], optarg);
                return -EINVAL;
        }

        if ( !priv->address )
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

    pdata->evt.domain_id = atoi(argv[optind]);
    /* assert(pdata->evt.domain_id) */

    return 0;
}

static int emul_unhandleable_init(xtf_monitor_t *pmon)
{
    int rc = 0;
    unsigned long gfn = 0x105;
    emul_unhandleable_test_t *pdata = (emul_unhandleable_test_t *)pmon->pdata;

    /* assert(pmon) */
    /* assert(pmon->pdata) */

    rc = xc_domain_set_access_required(pmon->xch, pmon->evt.domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d setting mem_access listener required\n", rc);
        return rc;
    }


    rc = xc_monitor_emul_unimplemented(pmon->xch, pmon->evt.domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d emulation unimplemented with vm_event\n", rc);
        return rc;
    }


    rc = xc_altp2m_set_domain_state(pmon->xch, pmon->evt.domain_id, 1);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d enabling altp2m on domain!\n", rc);
        return rc;
    }

    rc = xc_altp2m_create_view(pmon->xch, pmon->evt.domain_id,
            XENMEM_access_rwx, &pdata->altp2m_view_id );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d creating altp2m view!\n", rc);
        return rc;
    }

    rc = xc_altp2m_set_mem_access(pmon->xch, pmon->evt.domain_id,
            pdata->altp2m_view_id, 0x105, XENMEM_access_rw);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d setting altp2m memory access!\n", rc);
        return rc;
    }


    gfn = xc_translate_foreign_address(pmon->xch, pmon->evt.domain_id, 0, 0x105000);

    pdata->map = xc_map_foreign_range(pmon->xch, pmon->evt.domain_id, 4096,
            PROT_READ | PROT_WRITE , 0);
    if ( !pdata->map )
    {
        fprintf(stderr, "Failed to map page.\n");
        return -1;
    }

    rc = xc_altp2m_switch_to_view(pmon->xch, pmon->evt.domain_id, pdata->altp2m_view_id );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d switching to altp2m view!\n", rc);
        return rc;
    }

    rc = xc_monitor_singlestep(pmon->xch, pmon->evt.domain_id, 1 );
    if ( rc < 0 )
    {
        fprintf(stderr, "Error %d failed to enable singlestep monitoring!\n", rc);
        return rc;
    }

    return 0;
}

static int emul_unhandleable_cleanup(xtf_monitor_t *pmon)
{

    /* assert(pmon) */
    /* assert(pmon->pdata) */

    emul_unhandleable_test_t *pdata = (emul_unhandleable_test_t *)pmon->pdata;

    xc_altp2m_switch_to_view(pmon->xch, pmon->evt.domain_id, 0 );

    xc_altp2m_destroy_view(pmon->xch, pmon->evt.domain_id, pdata->altp2m_view_id);

    xc_altp2m_set_domain_state(pmon->xch, pmon->evt.domain_id, 0);

    xc_monitor_singlestep(pmon->xch, pmon->evt.domain_id, 0);

    if (pdata->map)
    {
        munmap(pdata->map, 4096);
    }

    return 0;
}

static emul_unhandleable_test_t test_data;

static xtf_monitor_ops_t emul_unhandleable_ops = { };

static xtf_monitor_t monitor_instance =
{
    .pdata      = (void *)&test_data,
    .setup      = emul_unhandleable_setup,
    .init       = emul_unhandleable_init,
    .cleanup    = emul_unhandleable_cleanup,
    .evt.ops    = &emul_unhandleable_ops,
};

xtf_monitor_t *pmon = &monitor_instance;

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
