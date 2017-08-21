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

xtf_monitor_t *xtf_monitor_instance;
xtf_monitor_t *get_monitor()
{
    return xtf_monitor_instance;
}

xtf_evtchn_t *get_evtchn(domid_t domain_id)
{
    (void)(domain_id);
    printf("monitor->evt = %p\n", monitor->evt);
    return monitor->evt;
}

int add_evtchn(xtf_evtchn_t *evt, domid_t domain_id)
{
    (void)(domain_id);
    monitor->evt = evt;
}

int xtf_evtchn_init(domid_t domain_id)
{
    int rc;
    xtf_evtchn_t *evt = evtchn(domain_id);

    printf("[DEBUG] xtf_evtchn_init\n");

    if ( !evt )
    {
        fprintf(stderr, "AAABP\n");
        return -EINVAL;
    }

    evt->ring_page = xc_monitor_enable(monitor->xch, domain_id,
            &evt->remote_port);
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

    rc = xenevtchn_bind_interdomain(evt->xce_handle, domain_id,
            evt->remote_port);
    if ( rc < 0 )
    {
        fprintf(stderr, "Failed to bind XEN event channel\n");
        return rc;
    }
    evt->local_port = rc;

    /* Initialise ring */
    SHARED_RING_INIT((vm_event_sring_t *)evt->ring_page);
    BACK_RING_INIT(&evt->back_ring, (vm_event_sring_t *)evt->ring_page,
            XC_PAGE_SIZE);
    return 0;
}

int xtf_evtchn_cleanup(domid_t domain_id)
{
    int rc;
    xtf_evtchn_t *evt = evtchn(domain_id);

    if ( !evt )
        return -EINVAL;

    if ( evt->ring_page )
        munmap(evt->ring_page, XC_PAGE_SIZE);

    rc = xc_monitor_disable(monitor->xch, domain_id);
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
        if (errno == EINTR) 
        {
            printf("poll : EINTR RECEIVED ");
            return 0;
        }

        printf("poll : received %d\n", errno);

        return -errno;
    }

    if ( rc == 1 )
    {
        port = xenevtchn_pending(xce);
        if ( port == -1 )
        {
            printf("xenevtchn_pending returned -1, errno=%d\n", errno);
            return -errno;
        }

        rc = xenevtchn_unmask(xce, port);
        if ( rc != 0 )
            return -errno;
    }
    else
        port = -1;

    return port;
}

static void xtf_evtchn_get_request(xtf_evtchn_t *evt, vm_event_request_t *req)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX req_cons;

    back_ring = &evt->back_ring;
    req_cons = back_ring->req_cons;

    /* Copy request */
    memcpy(req, RING_GET_REQUEST(back_ring, req_cons), sizeof(*req));
    req_cons++;

    /* Update ring */
    back_ring->req_cons = req_cons;
    back_ring->sring->req_event = req_cons + 1;
}

static void xtf_evtchn_put_response(xtf_evtchn_t *evt, vm_event_response_t *rsp)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX rsp_prod;

    back_ring = &evt->back_ring;
    rsp_prod = back_ring->rsp_prod_pvt;

    /* Copy response */
    memcpy(RING_GET_RESPONSE(back_ring, rsp_prod), rsp, sizeof(*rsp));
    rsp_prod++;

    /* Update ring */
    back_ring->rsp_prod_pvt = rsp_prod;
    RING_PUSH_RESPONSES(back_ring);
}

int xtf_evtchn_loop(domid_t domain_id)
{
    vm_event_request_t req;
    vm_event_response_t rsp;
    int rc;
    xtf_evtchn_t *evt = evtchn(domain_id);

    if ( !evt )
        return -EINVAL;

    printf("xtf_evtchn_loop\n");

    for (;;)
    {
        printf("Waiting for event ...");
        rc = xc_wait_for_event_or_timeout(xtf_xch, evt->xce_handle, 100);
        if ( rc < -1 )
        {
            fprintf(stderr, "Error getting event");
            return rc;
        }
        else if ( rc != -1 )
        {
            printf("Got event from Xen\n");
        }
        
        printf("rc = %d\n", rc);

        while ( RING_HAS_UNCONSUMED_REQUESTS(&evt->back_ring) )
        {
            xtf_evtchn_get_request(evt, &req);

            printf("Event received.\n");

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
                printf("Check1\n");
                if ( evt->ops->mem_access_handler )
                    rc = evt->ops->mem_access_handler(domain_id, &req, &rsp);
                break;
            case VM_EVENT_REASON_SINGLESTEP:
                printf("Check2\n");
                if ( evt->ops->singlestep_handler )
                    rc = evt->ops->singlestep_handler(domain_id, &req, &rsp);
                break;
            case VM_EVENT_REASON_EMUL_UNIMPLEMENTED:
                printf("Check3\n");
                if ( evt->ops->emul_unimpl_handler )
                    rc = evt->ops->emul_unimpl_handler(domain_id, &req, &rsp);
                break;
            default:
                fprintf(stderr, "Unknown request id = %d\n", req.reason);
            }

            if ( rc )
                return rc;

            /* Put the response on the ring */
            xtf_evtchn_put_response(evt, &rsp);
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

    printf("Gogu1\n");

    /* test specific setup sequence */
    rc = xtf_monitor_setup(argc, argv);
    if ( rc )
        return rc;

    printf("Gogu2\n");

    monitor->xch = xc_interface_open(NULL, NULL, 0);
    if ( !monitor->xch )
    {
        fprintf(stderr, "Error initialising xenaccess\n");
        return -1;
    }

    printf("Gogu3\n");
    /* test specific initialization sequence */
    rc = xtf_monitor_init();
    if ( rc )
        goto cleanup;

    printf("Gogu4\n");
    /* Run test */
    rc = xtf_monitor_run();
    if ( rc )
    {
        fprintf(stderr, "Error running test\n");
    }

cleanup:
    /* test specific cleanup sequence */
    xtf_monitor_cleanup();
    xc_interface_close(monitor->xch);

    printf("Gogu5 rc=%d\n", rc);
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
