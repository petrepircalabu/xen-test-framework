/**
 * @file monitor/monitor.c
 *
 * Common functions for test specific monitor applications.
 */

#include <monitor.h>
#include <stdio.h>
#include <sys/mman.h>

void usage()
{
    fprintf(stderr, "%s", monitor_test_help);
}

int monitor_init(xc_interface *xch, xtf_vm_event_t *event, domid_t domain_id)
{
    int rc;

    event->domain_id = domain_id;

    event->ring_page = xc_monitor_enable(xch, event->domain_id,
            &event->remote_port);
    if ( !event->ring_page )
    {
        fprintf(stderr, "Error enabling monitor\n");
        return -1;
    }

    event->xce_handle = xenevtchn_open(NULL, 0);
    if ( !event->xce_handle )
    {
        fprintf(stderr, "Failed to open XEN event channel\n");
        return -1;
    }

    rc = xenevtchn_bind_interdomain(event->xce_handle, event->domain_id,
            event->remote_port);
    if ( rc < 0 )
    {
        fprintf(stderr, "Failed to bind XEN event channel\n");
        return rc;
    }
    event->local_port = rc;

    /* Initialise ring */
    SHARED_RING_INIT((vm_event_sring_t *)event->ring_page);
    BACK_RING_INIT(&event->back_ring,
                   (vm_event_sring_t *)event->ring_page,
                   XC_PAGE_SIZE);
    return 0;
}

int monitor_cleanup(xc_interface *xch, xtf_vm_event_t *event)
{
    int rc;

    if ( event->ring_page )
        munmap(event->ring_page, XC_PAGE_SIZE);

    rc = xc_monitor_disable(xch, event->domain_id);
    if ( rc != 0 )
    {
        fprintf(stderr, "Error disabling monitor\n");
        return rc;
    }

    rc = xenevtchn_unbind(event->xce_handle, event->local_port);
    if ( rc != 0 )
    {
        fprintf(stderr, "Failed to unbind XEN event channel\n");
        return rc;
    }

    rc = xenevtchn_close(event->xce_handle);
    if ( rc != 0 )
    {
        printf("Failed to close XEN event channel\n");
        return rc;
    }

    return 0;
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
