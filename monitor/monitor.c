#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <xenctrl.h>
#include <xenevtchn.h>
#include <xen/vm_event.h>

typedef struct xen_monitor_test
{
    xc_interface *xch;

    domid_t domain_id;

    void *ring_page;

    uint32_t evtchn_port;

    xenevtchn_handle *xce_handle;

    int port;

    vm_event_back_ring_t back_ring;

    xen_pfn_t max_gpfn;

} xen_monitor_test_t;

static int monitor_init(xen_monitor_test_t *test, domid_t domain_id)
{
    int rc;

    test->xch = xc_interface_open(NULL, NULL, 0);
    if ( !test->xch )
    {
        printf("Error initialising xenaccess\n");
        return -1;
    }

    test->domain_id = domain_id;

    test->ring_page = xc_monitor_enable(test->xch, test->domain_id,
            &test->evtchn_port);
    if ( !test->ring_page )
    {
        printf("Error enabling monitor\n");
        return -1;
    }

    test->xce_handle = xenevtchn_open(NULL, 0);
    if ( !test->xce_handle )
    {
        printf("Failed to open XEN event channel\n");
        return -1;
    }

    rc = xenevtchn_bind_interdomain(test->xce_handle,
            test->domain_id, test->evtchn_port);
    if ( rc < 0 )
    {
        printf("Failed to bind XEN event channel\n");
        return -1;
    }
    test->port = rc;

    /* Initialise ring */
    SHARED_RING_INIT((vm_event_sring_t *)test->ring_page);
    BACK_RING_INIT(&test->back_ring,
                   (vm_event_sring_t *)test->ring_page,
                   XC_PAGE_SIZE);
    return 0;
}

static int monitor_cleanup(xen_monitor_test_t *test)
{
    int rc;

    if ( test->ring_page )
        munmap(test->ring_page, XC_PAGE_SIZE);

    rc = xc_monitor_disable(test->xch, test->domain_id);
    if ( rc != 0 )
    {
        printf("Error disabling monitor\n");
        return rc;
    }

    rc = xenevtchn_unbind(test->xce_handle, test->port);
    if ( rc != 0 )
    {
        printf("Failed to unbind XEN event channel\n");
        return rc;
    }

    rc = xenevtchn_close(test->xce_handle);
    if ( rc != 0 )
    {
        printf("Failed to close XEN event channel\n");
        return rc;
    }
    
    if ( test->xch )
    {
        rc = xc_interface_close(test->xch);
        if ( rc != 0 )
        {
            printf("Error closing connection to XEN.\n");
            return rc;
        }
    }

    return 0;
}

int monitor_test()
{
    return 0;
}

int main(int argc, char* argv[])
{
    domid_t domain_id;
    int rc;

    xen_monitor_test_t *test = calloc(1, sizeof(xen_monitor_test_t));
    if (!test)
    {
        printf("Failed to allocate memory\n");
        return -1;
    }

    domain_id = atoi(argv[1]);
    printf("Domain ID=%d\n", domain_id);

    monitor_init(test, domain_id);

    /* Get max_gpfn */
    rc = xc_domain_maximum_gpfn(test->xch,
                                test->domain_id,
                                &test->max_gpfn);
    if ( rc )
    {
        printf("Failed to get max gpfn");
    }

    printf("max_gpfn = %"PRI_xen_pfn"\n", test->max_gpfn);

    rc = monitor_test();
    if ( rc )
    {
        printf("");
    }

cleanup:
    monitor_cleanup(test);

    free(test);
    return 0;
}
