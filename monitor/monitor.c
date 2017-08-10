#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    uint16_t altp2m_view_id;

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

int xc_wait_for_event_or_timeout(xc_interface *xch, xenevtchn_handle *xce, unsigned long ms)
{
    struct pollfd fd = { .fd = xenevtchn_fd(xce), .events = POLLIN | POLLERR };
    int port;
    int rc;

    rc = poll(&fd, 1, ms);
    if ( rc == -1 )
    {
        if (errno == EINTR)
            return 0;

        printf("Poll exited with an error");
        goto err;
    }

    if ( rc == 1 )
    {
        port = xenevtchn_pending(xce);
        if ( port == -1 )
        {
            printf("Failed to read port from event channel");
            goto err;
        }

        rc = xenevtchn_unmask(xce, port);
        if ( rc != 0 )
        {
            printf("Failed to unmask event channel port");
            goto err;
        }
    }
    else
        port = -1;

    return port;

 err:
    return -errno;
}

static void get_request(xen_monitor_test_t *test, vm_event_request_t *req)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX req_cons;

    back_ring = &test->back_ring;
    req_cons = back_ring->req_cons;

    /* Copy request */
    memcpy(req, RING_GET_REQUEST(back_ring, req_cons), sizeof(*req));
    req_cons++;

    /* Update ring */
    back_ring->req_cons = req_cons;
    back_ring->sring->req_event = req_cons + 1;
}

static void put_response(xen_monitor_test_t *test, vm_event_response_t *rsp)
{
    vm_event_back_ring_t *back_ring;
    RING_IDX rsp_prod;

    back_ring = &test->back_ring;
    rsp_prod = back_ring->rsp_prod_pvt;

    /* Copy response */
    memcpy(RING_GET_RESPONSE(back_ring, rsp_prod), rsp, sizeof(*rsp));
    rsp_prod++;

    /* Update ring */
    back_ring->rsp_prod_pvt = rsp_prod;
    RING_PUSH_RESPONSES(back_ring);
}

int monitor_test(xen_monitor_test_t *test)
{
    vm_event_request_t req;
    vm_event_response_t rsp;
    int rc;
    unsigned long instr = 0;

    printf ("Running monitor test...\n");
    for (;;)
    {
        rc = xc_wait_for_event_or_timeout(test->xch, test->xce_handle, 100);
        if ( rc < -1 )
        {
            printf("Error getting event");
            return -1;
        }
        else if ( rc != -1 )
        {
            printf("Got event from Xen\n");
        }

        while ( RING_HAS_UNCONSUMED_REQUESTS(&test->back_ring) )
        {
            get_request(test, &req);

            if ( req.version != VM_EVENT_INTERFACE_VERSION )
            {
                printf("Error: vm_event interface version mismatch!\n");
                return -1;
            }

            memset( &rsp, 0, sizeof (rsp) );
            rsp.version = VM_EVENT_INTERFACE_VERSION;
            rsp.vcpu_id = req.vcpu_id;
            rsp.flags = (req.flags & VM_EVENT_FLAG_VCPU_PAUSED);
            rsp.reason = req.reason;

            switch (req.reason) 
            {
            case VM_EVENT_REASON_MEM_ACCESS:
                printf("mem_access rip = %016x gfn = %lx offset = %lx gla =%lx\n",
                   req.data.regs.x86.rip,
                   req.u.mem_access.gfn,
                   req.u.mem_access.offset,
                   req.u.mem_access.gla);
                //if ( req.flags & VM_EVENT_FLAG_ALTERNATE_P2M )
                {
                    printf("\tSwitching back to default view!\n");

                    //rsp.flags |= VM_EVENT_FLAG_ALTERNATE_P2M | VM_EVENT_FLAG_TOGGLE_SINGLESTEP;
                    rsp.flags |= VM_EVENT_FLAG_EMULATE;
                    //rsp.altp2m_idx = 0;
                    {
                        volatile unsigned long *p = (volatile unsigned long *)req.data.regs.x86.rip;
                        instr = *p;
                        *p = 0xDEADBABE;
                    }
                }
                //rsp.u.mem_access = req.u.mem_access;
                break;
            case VM_EVENT_REASON_SINGLESTEP:
                printf("Singlestep: rip=%016lx, vcpu %d, altp2m %u\n",
                       req.data.regs.x86.rip,
                       req.vcpu_id,
                       req.altp2m_idx);

                printf("\tSwitching altp2m to view %u!\n", 
                        test->altp2m_view_id);

                rsp.flags |= VM_EVENT_FLAG_ALTERNATE_P2M;
                rsp.altp2m_idx = test->altp2m_view_id;

                rsp.flags |= VM_EVENT_FLAG_TOGGLE_SINGLESTEP;

                break;
            case VM_EVENT_REASON_EMUL_UNIMPLEMENTED:

                printf("Emulation unimplemented: rip=%016lx, vcpu %d:\n",
                       req.data.regs.x86.rip,
                       req.vcpu_id);

                {
                    volatile unsigned long *p = (volatile unsigned long *)req.data.regs.x86.rip;
                    *p = instr;
                }
                rsp.flags |= (VM_EVENT_FLAG_ALTERNATE_P2M | VM_EVENT_FLAG_TOGGLE_SINGLESTEP);
                rsp.altp2m_idx = 0;
                break;
            default:
                printf ("Unknown request id = %d\n", req.reason);
            }

            /* Put the response on the ring */
            put_response(test, &rsp);
        }
        /* Tell Xen page is ready */
        rc = xenevtchn_notify(test->xce_handle,
                              test->port);

        if ( rc != 0 )
        {
            printf("Error resuming page");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    domid_t domain_id;
    int rc, i;
    xenmem_access_t tmp, page;

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
        goto cleanup;
    }

    printf("max_gpfn = %"PRI_xen_pfn"\n", test->max_gpfn);


    /* Set whether the access listener is required */
    rc = xc_domain_set_access_required(test->xch, test->domain_id, 0);
    if ( rc < 0 )
    {
        printf("Error %d setting mem_access listener required\n", rc);
        goto cleanup;
    }

    /*
    for ( i = 0; i < test->max_gpfn; i++ )
    {
        if ( xc_get_mem_access(test->xch, test->domain_id,
                    i, &page) != 0 )
        {
   //         printf("Error getting mem access for page %d.\n", i);
   //         goto cleanup;
            continue;
        }

        printf ("Page %d is %d.\n", i, page);

        if ( page == XENMEM_access_rx )
        {
            printf("Page %d is executable.\n", i);
        }
    }
    */
    unsigned long perm_set = 0;

    rc = xc_altp2m_set_domain_state( test->xch, test->domain_id, 1 );
    if ( rc < 0 )
    {
        printf("Error %d enabling altp2m on domain!\n", rc);
        goto cleanup;
    }

    rc = xc_altp2m_create_view( test->xch, test->domain_id, XENMEM_access_rwx, &test->altp2m_view_id );
    if ( rc < 0 )
    {
        printf("Error %d creating altp2m view!\n", rc);
        goto cleanup;
    }

    printf("altp2m view created with id %u\n", test->altp2m_view_id);
    printf("Setting altp2m mem_access permissions.. ");

//    for(i=0; i < test->max_gpfn; ++i)
    {
        rc = xc_altp2m_set_mem_access( test->xch, test->domain_id, test->altp2m_view_id, 
                                       0x105, //FIXME:
                                       XENMEM_access_rw);
        if ( !rc )
            perm_set++;
    }

    printf("done! Permissions set on %lu pages.\n", perm_set);

    rc = xc_altp2m_switch_to_view( test->xch, test->domain_id, test->altp2m_view_id );
    if ( rc < 0 )
    {
        printf("Error %d switching to altp2m view!\n", rc);
        goto cleanup;
    }

    rc = xc_monitor_singlestep( test->xch, test->domain_id, 1 );
    if ( rc < 0 )
    {
        printf("Error %d failed to enable singlestep monitoring!\n", rc);
        goto cleanup;
    }
    /* Unpause the domain and start test */
    rc = xc_domain_unpause(test->xch, test->domain_id);
    if ( rc < 0 )
    {
        printf("Error unpausing domain.");
        goto cleanup;
    }

    rc = monitor_test(test);
    if ( rc )
    {
        printf("  Failed to run test");
    }

cleanup:
    rc = xc_altp2m_switch_to_view(test->xch, test->domain_id, 0 );
    rc = xc_altp2m_destroy_view(test->xch, test->domain_id, test->altp2m_view_id);
    rc = xc_altp2m_set_domain_state(test->xch, test->domain_id, 0);
    rc = xc_monitor_singlestep(test->xch, test->domain_id, 0);
    monitor_cleanup(test);

    free(test);
    return 0;
}
