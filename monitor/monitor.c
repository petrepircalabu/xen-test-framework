/**
 * @file monitor/monitor.c
 *
 * Common functions for test specific monitor applications.
 */

#include <errno.h>
#include <monitor.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

void usage()
{
    fprintf(stderr, "%s", monitor_test_help);
}

int xtf_monitor_init(xc_interface *xch, xtf_vm_event_t *evt)
{
    int rc;

    evt->ring_page = xc_monitor_enable(xch, evt->domain_id, &evt->remote_port);
    if ( !evt->ring_page )
    {
        fprintf(stderr, "Error enabling monitor\n");
        return -1;
    }

    evt->xce_handle = xenevtchn_open(NULL, 0);
    if ( !evt->xce_handle )
    {
        fprintf(stderr, "Failed to open XEN event channel\n");
        return -1;
    }

    rc = xenevtchn_bind_interdomain(evt->xce_handle, evt->domain_id, evt->remote_port);
    if ( rc < 0 )
    {
        fprintf(stderr, "Failed to bind XEN event channel\n");
        return rc;
    }
    evt->local_port = rc;

    /* Initialise ring */
    SHARED_RING_INIT((vm_event_sring_t *)evt->ring_page);
    BACK_RING_INIT(&evt->back_ring, (vm_event_sring_t *)evt->ring_page, XC_PAGE_SIZE);
    return 0;
}

int xtf_monitor_cleanup(xc_interface *xch, xtf_vm_event_t *evt)
{
    int rc;

    if ( evt->ring_page )
        munmap(evt->ring_page, XC_PAGE_SIZE);

    rc = xc_monitor_disable(xch, evt->domain_id);
    if ( rc != 0 )
    {
        fprintf(stderr, "Error disabling monitor\n");
        return rc;
    }

    rc = xenevtchn_unbind(evt->xce_handle, evt->local_port);
    if ( rc != 0 )
    {
        fprintf(stderr, "Failed to unbind XEN event channel\n");
        return rc;
    }

    rc = xenevtchn_close(evt->xce_handle);
    if ( rc != 0 )
    {
        printf("Failed to close XEN event channel\n");
        return rc;
    }

    return 0;
}

static int xc_wait_for_event_or_timeout(xc_interface *xch, xenevtchn_handle *xce, unsigned long ms)
{
    struct pollfd fd = { .fd = xenevtchn_fd(xce), .events = POLLIN | POLLERR };
    int port;
    int rc;

    rc = poll(&fd, 1, ms);
    if ( rc == -1 )
    {
        return (errno == EINTR) ? 0 : -errno;
    }

    if ( rc == 1 )
    {
        port = xenevtchn_pending(xce);
        if ( port == -1 )
            return -errno;

        rc = xenevtchn_unmask(xce, port);
        if ( rc != 0 )
            return -errno;
    }
    else
        port = -1;

    return port;
}

static void xtf_monitor_get_request(xtf_vm_event_t *event, vm_event_request_t *req)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX req_cons;

    back_ring = &event->back_ring;
    req_cons = back_ring->req_cons;

    /* Copy request */
    memcpy(req, RING_GET_REQUEST(back_ring, req_cons), sizeof(*req));
    req_cons++;

    /* Update ring */
    back_ring->req_cons = req_cons;
    back_ring->sring->req_event = req_cons + 1;
}

static void xtf_monitor_put_response(xtf_vm_event_t *event, vm_event_response_t *rsp)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX rsp_prod;

    back_ring = &event->back_ring;
    rsp_prod = back_ring->rsp_prod_pvt;

    /* Copy response */
    memcpy(RING_GET_RESPONSE(back_ring, rsp_prod), rsp, sizeof(*rsp));
    rsp_prod++;

    /* Update ring */
    back_ring->rsp_prod_pvt = rsp_prod;
    RING_PUSH_RESPONSES(back_ring);
}

int xtf_monitor_loop(xc_interface *xch, xtf_vm_event_t *evt)
{
    vm_event_request_t req;
    vm_event_response_t rsp;
    int rc;

    for (;;)
    {
        rc = xc_wait_for_event_or_timeout(xch, evt->xce_handle, 100);
        if ( rc < -1 )
        {
            fprintf(stderr, "Error getting event");
            return rc;
        }

        while ( RING_HAS_UNCONSUMED_REQUESTS(&evt->back_ring) )
        {
            xtf_monitor_get_request(evt, &req);

            if ( req.version != VM_EVENT_INTERFACE_VERSION )
            {
                fprintf(stderr, "Error: vm_event interface version mismatch!\n");
                return -1;
            }

            memset( &rsp, 0, sizeof (rsp) );
            rsp.version = VM_EVENT_INTERFACE_VERSION;
            rsp.vcpu_id = req.vcpu_id;
            rsp.flags = (req.flags & VM_EVENT_FLAG_VCPU_PAUSED);
            rsp.reason = req.reason;

            rc = 0;

            switch (req.reason)
            {
            case VM_EVENT_REASON_MEM_ACCESS:
                if ( evt->ops->mem_access_handler )
                    rc = evt->ops->mem_access_handler();
                break;
            case VM_EVENT_REASON_SINGLESTEP:
                if ( evt->ops->singlestep_handler )
                    rc = evt->ops->singlestep_handler();
                break;
            case VM_EVENT_REASON_EMUL_UNIMPLEMENTED:
                if ( evt->ops->emul_unimpl_handler )
                    evt->ops->emul_unimpl_handler();
                break;
            default:
                fprintf(stderr, "Unknown request id = %d\n", req.reason);
            }

            /* Put the response on the ring */
            xtf_monitor_put_response(evt, &rsp);
        }
        /* Tell Xen page is ready */
        rc = xenevtchn_notify(evt->xce_handle, evt->local_port);

        if ( rc != 0 )
        {
            fprintf(stderr, "Error resuming page");
            return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int rc;

    /* test specific setup sequence */
    if ( pmon->setup )
    {
        rc = pmon->setup(pmon, argc, argv);
        if ( rc )
            return rc;
    }

    pmon->xch = xc_interface_open(NULL, NULL, 0);
    if ( !pmon->xch )
    {
        fprintf(stderr, "Error initialising xenaccess\n");
        return -1;
    }

    /* generic initialization sequence */
    rc = xtf_monitor_init(pmon->xch, &pmon->evt);
    if ( rc )
        goto cleanup;

    /* test specific initialization sequence */
    if (pmon->init)
    {
        rc = pmon->init(pmon);
        if ( rc )
            goto cleanup;
    }

    /* Unpause the domain and start test */
    rc = xc_domain_unpause(pmon->xch, pmon->evt.domain_id);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error unpausing domain.\n");
        goto cleanup;
    }

    /* Event loop */
    rc = xtf_monitor_loop(pmon->xch, &pmon->evt);
    if ( rc < 0 )
    {
        fprintf(stderr, "Error running event loop\n");
    }

cleanup:
    /* test specific cleanup sequence */
    if ( pmon->cleanup )
        pmon->cleanup(pmon);

    /* generic cleanup sequence */
    xtf_monitor_cleanup(pmon->xch, &pmon->evt);

    if ( pmon->xch )
    {
        xc_interface_close(pmon->xch);
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
