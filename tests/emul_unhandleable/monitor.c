/**
 * @file tests/emul_unhandleable/monitor.c
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct emul_unhandleable_test
{
    xtf_vm_event_t vm_event;
    xc_interface *xch;
    uint64_t address;
} emul_unhandleable_test_t;

static emul_unhandleable_test_t test_data;

const char monitor_test_help[] = \
    "Usage: test-monitor-emul_unhandleable [options] <domid>\n"
    "\t -a <address>: the address where an invalid instruction will be injected\n"
    ;

static int parse_options(void *pdata, int argc, char *argv[])
{
    int ret, c;
    static struct option long_options[] = {
        MONITOR_COMMON_OPTIONS_LONG,
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
        c = getopt_long(argc, argv, MONITOR_COMMON_OPTIONS_SHORT "a:",
                long_options, &option_index);
        if ( c == -1 ) break;

        switch ( c )
        {
            MONITOR_COMMON_OPTIONS_HANDLER;
            case 'a':
                priv->address = atol(optarg);
                break;
            default:
                fprintf(stderr, "%s: Invalid option %s\n", argv[0], optarg);
                return -EINVAL;
        }

        if ( !priv->address )
        {
            fprintf(stderr, "%s: Please specify a valid instruction injection address\n");
            return -EINVAL;
        }

        if ( optind != argc - 1 )
        {
            fprintf(stderr, "%s: Please specify the domain id\n");
            return -EINVAL;
        }
    }

    priv->vm_event.domain_id = atoi(argv[optind]);

    return 0;
}

static int emul_unhandleable_init(emul_unhandleable_test_t *priv)
{
    return 0;
}

static int emul_unhandleable_cleanup(emul_unhandleable_test_t *priv)
{
    return 0;
}

int main(int argc, char* argv[])
{
    int rc;
    rc = parse_options((void *)&test_data, argc, argv);
    if ( rc )
        return rc;

    test_data.xch = xc_interface_open(NULL, NULL, 0);
    if ( !test_data.xch )
    {
        fprintf(stderr, "Error initialising xenaccess\n");
        return -1;
    }

    rc = monitor_init(test_data.xch, &test_data.vm_event, test_data.vm_event.domain_id);
    if ( rc )
        goto e_exit;

    rc = emul_unhandleable_init(&test_data);
    if ( rc )
        goto e_exit;

e_exit:
    emul_unhandleable_cleanup(&test_data);

    monitor_cleanup(test_data.xch, &test_data.vm_event);

    if ( test_data.xch )
    {
        xc_interface_close(test_data.xch);
    }
    return rc;
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
