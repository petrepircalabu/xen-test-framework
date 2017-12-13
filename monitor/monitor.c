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

void xtf_log(xtf_mon_log_level_t lvl, const char *fmt, ...)
{
    static const char* log_level_names[] = {
        "FATAL",
        "ERROR",
        "WARNING",
        "INFO",
        "DEBUG",
        "TRACE",
    };

    if ( lvl <= monitor->log_lvl )
    {
        va_list argptr;

        fprintf(stderr, "[%s]\t", log_level_names[lvl]);
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
    }
}

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

    if ( !evt )
    {
        XTF_MON_ERROR("Invalid event channel\n");
        return -EINVAL;
    }

    evt->ring_page = xc_monitor_enable(monitor->xch, domain_id,
            &evt->remote_port);
    if ( !evt->ring_page )
    {
        XTF_MON_ERROR("Error enabling monitor\n");
        return -1;
    }

    evt->xce_handle = xenevtchn_open(NULL, 0);
    if ( !evt->xce_handle )
    {
        XTF_MON_ERROR("Failed to open XEN event channel\n");
        return -1;
    }

    rc = xenevtchn_bind_interdomain(evt->xce_handle, domain_id,
            evt->remote_port);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Failed to bind XEN event channel\n");
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
        XTF_MON_INFO("Error disabling monitor\n");
        return rc;
    }

    rc = xenevtchn_unbind(evt->xce_handle, evt->local_port);
    if ( rc != 0 )
    {
        XTF_MON_INFO("Failed to unbind XEN event channel\n");
        return rc;
    }

    rc = xenevtchn_close(evt->xce_handle);
    if ( rc != 0 )
    {
        XTF_MON_INFO("Failed to close XEN event channel\n");
        return rc;
    }

    return 0;
}

static int xtf_wait_for_event(domid_t domain_id, xc_interface *xch, xenevtchn_handle *xce, unsigned long ms)
{
    struct pollfd fds[2] = {
        { .fd = xenevtchn_fd(xce), .events = POLLIN | POLLERR },
        { .fd = xs_fileno(monitor->xsh), .events = POLLIN | POLLERR },
    };
    int port;
    int rc;

    rc = poll(fds, 2, ms);

    if ( rc < 0 )
        return -errno;

    if ( rc == 0 )
        return 0;

    if ( fds[0].revents )
    {
        port = xenevtchn_pending(xce);
        if ( port == -1 )
            return -errno;

        rc = xenevtchn_unmask(xce, port);
        if ( rc != 0 )
            return -errno;

        return port;
    }

    if ( fds[1].revents )
    {
        if ( !xs_is_domain_introduced(monitor->xsh, domain_id) )
        {
            return 1;
        }

        return 0;
    }

    return -2;  /* Error */
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

    printf("Monitor initialization complete.\n");

    for (;;)
    {
        rc = xtf_wait_for_event(domain_id, xtf_xch, evt->xce_handle, 100);
        if ( rc < -1 )
        {
            XTF_MON_ERROR("Error getting event");
            return rc;
        }
        else if ( rc == 1 )
        {
            XTF_MON_INFO("Domain %d exited\n", domain_id);
            return 0;
        }
        
        while ( RING_HAS_UNCONSUMED_REQUESTS(&evt->back_ring) )
        {
            xtf_evtchn_get_request(evt, &req);

            if ( req.version != VM_EVENT_INTERFACE_VERSION )
            {
                XTF_MON_ERROR("Error: vm_event interface version mismatch!\n");
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
                XTF_MON_DEBUG("mem_access rip = %016lx gfn = %lx offset = %lx gla =%lx\n",
                    req.data.regs.x86.rip,
                    req.u.mem_access.gfn,
                    req.u.mem_access.offset,
                    req.u.mem_access.gla);

                if ( evt->ops.mem_access_handler )
                    rc = evt->ops.mem_access_handler(domain_id, &req, &rsp);
                break;
            case VM_EVENT_REASON_SINGLESTEP:
                XTF_MON_DEBUG("Singlestep: rip=%016lx, vcpu %d, altp2m %u\n",
                    req.data.regs.x86.rip,
                    req.vcpu_id,
                    req.altp2m_idx);
                if ( evt->ops.singlestep_handler )
                    rc = evt->ops.singlestep_handler(domain_id, &req, &rsp);
                break;
            case VM_EVENT_REASON_EMUL_UNIMPLEMENTED:
                XTF_MON_DEBUG("Emulation unimplemented: rip=%016lx, vcpu %d:\n",
                    req.data.regs.x86.rip,
                    req.vcpu_id);
                if ( evt->ops.emul_unimpl_handler )
                    rc = evt->ops.emul_unimpl_handler(domain_id, &req, &rsp);
                break;
            default:
                XTF_MON_ERROR("Unknown request id = %d\n", req.reason);
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
            XTF_MON_ERROR("Error resuming page");
            return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int rc;

    /* test specific setup sequence */
    rc = xtf_monitor_setup(argc, argv);
    if ( rc )
        return rc;

    monitor->xch = xc_interface_open(NULL, NULL, 0);
    if ( !monitor->xch )
    {
        XTF_MON_FATAL("Error initialising xenaccess\n");
        return -1;
    }

    monitor->log_lvl = XTF_MON_LEVEL_ERROR;

    monitor->xsh = xs_open(XS_OPEN_READONLY);
    if ( !monitor->xsh )
    {
        XTF_MON_FATAL("Error opening XEN store\n");
        rc = -EINVAL;
        goto cleanup;
    }

    if ( !xs_watch( monitor->xsh, "@releaseDomain", "RELEASE_TOKEN") )
    {
        XTF_MON_FATAL("Error monitoring releaseDomain\n");
        rc = -EINVAL;
        goto cleanup;
    }

    /* test specific initialization sequence */
    rc = xtf_monitor_init();
    if ( rc )
        goto cleanup;

    /* Run test */
    rc = xtf_monitor_run();
    if ( rc )
    {
        XTF_MON_ERROR("Error running test\n");
    }

cleanup:
    /* test specific cleanup sequence */
    xtf_monitor_cleanup();
    if ( monitor->xsh )
    {
        xs_unwatch(monitor->xsh, "@releaseDomain", "RELEASE_TOKEN");
        xs_close(monitor->xsh);
        monitor->xsh = NULL;
    }

    xc_interface_close(monitor->xch);

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
