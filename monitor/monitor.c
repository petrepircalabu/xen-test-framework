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

#define call_helper(func, ... )         ( (func) ? func(__VA_ARGS__) : 0 )

static const char *status_to_str[] =
{
#define STA(x) [XTF_MON_STATUS_ ## x] = #x

    STA(RUNNING),
    STA(SUCCESS),
    STA(SKIP),
    STA(ERROR),
    STA(FAILURE),

#undef STA
};

static const char *log_level_to_str[] =
{
#define XTFMLL(x) [ XTF_MON_LOG_LEVEL_ ## x] = #x

    XTFMLL(FATAL),
    XTFMLL(ERROR),
    XTFMLL(WARNING),
    XTFMLL(INFO),
    XTFMLL(DEBUG),
    XTFMLL(TRACE),

#undef XTFMLL
};

void xtf_log(enum xtf_mon_log_level lvl, const char *fmt, ...)
{
    va_list argptr;

    if ( lvl < 0 || lvl > monitor->log_lvl )
        return;

    fprintf(stderr, "[%s]\t", log_level_to_str[lvl]);
    va_start(argptr, fmt);
    vfprintf(stderr, fmt, argptr);
    va_end(argptr);
}

static void xtf_print_status(enum xtf_mon_status s)
{
    if ( s > XTF_MON_STATUS_RUNNING && s <= XTF_MON_STATUS_FAILURE )
        printf("Test result: %s\n", status_to_str[s]);
}

void usage()
{
    fprintf(stderr, "%s", monitor->help_message);
}

xtf_monitor_t *monitor;

static int xtf_monitor_init()
{
    int rc;

    monitor->ring.ring_page = xc_monitor_enable(monitor->xch,
                                                monitor->domain_id,
                                                &monitor->ring.remote_port);
    if ( !monitor->ring.ring_page )
    {
        XTF_MON_ERROR("Error enabling monitor\n");
        return -1;
    }

    monitor->ring.xce_handle = xenevtchn_open(NULL, 0);
    if ( !monitor->ring.xce_handle )
    {
        XTF_MON_ERROR("Failed to open XEN event channel\n");
        return -1;
    }

    rc = xenevtchn_bind_interdomain(monitor->ring.xce_handle,
                                    monitor->domain_id,
                                    monitor->ring.remote_port);
    if ( rc < 0 )
    {
        XTF_MON_ERROR("Failed to bind XEN event channel\n");
        return rc;
    }
    monitor->ring.local_port = rc;

    /* Initialise ring */
    SHARED_RING_INIT((vm_event_sring_t *)monitor->ring.ring_page);
    BACK_RING_INIT(&monitor->ring.back_ring,
                   (vm_event_sring_t *)monitor->ring.ring_page,
                   XC_PAGE_SIZE);

    return 0;
}

static int xtf_monitor_cleanup()
{
    int rc;

    if ( monitor->ring.ring_page )
        munmap(monitor->ring.ring_page, XC_PAGE_SIZE);

    rc = xc_monitor_disable(monitor->xch, monitor->domain_id);
    if ( rc != 0 )
    {
        XTF_MON_INFO("Error disabling monitor\n");
        return rc;
    }

    rc = xenevtchn_unbind(monitor->ring.xce_handle, monitor->ring.local_port);
    if ( rc != 0 )
    {
        XTF_MON_INFO("Failed to unbind XEN event channel\n");
        return rc;
    }

    rc = xenevtchn_close(monitor->ring.xce_handle);
    if ( rc != 0 )
    {
        XTF_MON_INFO("Failed to close XEN event channel\n");
        return rc;
    }

    return 0;
}

static int xtf_monitor_wait_for_event(unsigned long ms)
{
    int rc;
    struct pollfd fds[2] = {
        { .fd = xenevtchn_fd(monitor->ring.xce_handle), .events = POLLIN | POLLERR },
        { .fd = xs_fileno(monitor->xsh), .events = POLLIN | POLLERR },
    };

    rc = poll(fds, 2, ms);

    if ( rc < 0 )
        return -errno;

    if ( rc == 0 )
        return 0;

    if ( fds[0].revents )
    {
        int port = xenevtchn_pending(monitor->ring.xce_handle);
        if ( port == -1 )
            return -errno;

        rc = xenevtchn_unmask(monitor->ring.xce_handle, port);
        if ( rc != 0 )
            return -errno;

        return port;
    }

    if ( fds[1].revents )
    {
        if ( !xs_is_domain_introduced(monitor->xsh, monitor->domain_id) )
        {
            return 1;
        }

        return 0;
    }

    return -2;  /* Error */
}

/*
 * X86 control register names
 */
static const char* get_x86_ctrl_reg_name(uint32_t index)
{
    switch (index)
    {
        case VM_EVENT_X86_CR0:
            return "CR0";
        case VM_EVENT_X86_CR3:
            return "CR3";
        case VM_EVENT_X86_CR4:
            return "CR4";
        case VM_EVENT_X86_XCR0:
            return "XCR0";
    }
    return "";
}

static void xtf_monitor_dump_request(enum xtf_mon_log_level lvl, vm_event_request_t *req)
{
    switch ( req->reason )
    {
    case VM_EVENT_REASON_MEM_ACCESS:
        xtf_log(lvl, "PAGE ACCESS: %c%c%c for GFN %"PRIx64" (offset %06"
                PRIx64") gla %016"PRIx64" (valid: %c; fault in gpt: %c; fault with gla: %c) (vcpu %u [%c], altp2m view %u)\n",
                (req->u.mem_access.flags & MEM_ACCESS_R) ? 'r' : '-',
                (req->u.mem_access.flags & MEM_ACCESS_W) ? 'w' : '-',
                (req->u.mem_access.flags & MEM_ACCESS_X) ? 'x' : '-',
                req->u.mem_access.gfn,
                req->u.mem_access.offset,
                req->u.mem_access.gla,
                (req->u.mem_access.flags & MEM_ACCESS_GLA_VALID) ? 'y' : 'n',
                (req->u.mem_access.flags & MEM_ACCESS_FAULT_IN_GPT) ? 'y' : 'n',
                (req->u.mem_access.flags & MEM_ACCESS_FAULT_WITH_GLA) ? 'y': 'n',
                req->vcpu_id,
                (req->flags & VM_EVENT_FLAG_VCPU_PAUSED) ? 'p' : 'r',
                req->altp2m_idx);
        break;

    case VM_EVENT_REASON_SOFTWARE_BREAKPOINT:
        xtf_log(lvl, "Breakpoint: rip=%016"PRIx64", gfn=%"PRIx64" (vcpu %d)\n",
                req->data.regs.x86.rip,
                req->u.software_breakpoint.gfn,
                req->vcpu_id);
        break;

    case VM_EVENT_REASON_PRIVILEGED_CALL:
        xtf_log(lvl, "Privileged call: pc=%"PRIx64" (vcpu %d)\n",
                req->data.regs.arm.pc,
                req->vcpu_id);

    case VM_EVENT_REASON_SINGLESTEP:
        xtf_log(lvl, "Singlestep: rip=%016lx, vcpu %d, altp2m %u\n",
                req->data.regs.x86.rip,
                req->vcpu_id,
                req->altp2m_idx);
        break;

    case VM_EVENT_REASON_DEBUG_EXCEPTION:
        printf("Debug exception: rip=%016"PRIx64", vcpu %d. Type: %u. Length: %u\n",
                req->data.regs.x86.rip,
                req->vcpu_id,
                req->u.debug_exception.type,
                req->u.debug_exception.insn_length);
        break;

    case VM_EVENT_REASON_CPUID:
        xtf_log(lvl, "CPUID executed: rip=%016"PRIx64", vcpu %d. Insn length: %"PRIu32" " \
                "0x%"PRIx32" 0x%"PRIx32": EAX=0x%"PRIx64" EBX=0x%"PRIx64" ECX=0x%"PRIx64" EDX=0x%"PRIx64"\n",
                req->data.regs.x86.rip,
                req->vcpu_id,
                req->u.cpuid.insn_length,
                req->u.cpuid.leaf,
                req->u.cpuid.subleaf,
                req->data.regs.x86.rax,
                req->data.regs.x86.rbx,
                req->data.regs.x86.rcx,
                req->data.regs.x86.rdx);
        break;

    case VM_EVENT_REASON_DESCRIPTOR_ACCESS:
        xtf_log(lvl, "Descriptor access: rip=%016"PRIx64", vcpu %d: "\
                "VMExit info=0x%"PRIx32", descriptor=%d, is write=%d\n",
                req->data.regs.x86.rip,
                req->vcpu_id,
                req->u.desc_access.arch.vmx.instr_info,
                req->u.desc_access.descriptor,
                req->u.desc_access.is_write);
        break;

    case VM_EVENT_REASON_WRITE_CTRLREG:
        xtf_log(lvl,"Control register written: rip=%016"PRIx64", vcpu %d: "
                "reg=%s, old_value=%016"PRIx64", new_value=%016"PRIx64"\n",
                req->data.regs.x86.rip,
                req->vcpu_id,
                get_x86_ctrl_reg_name(req->u.write_ctrlreg.index),
                req->u.write_ctrlreg.old_value,
                req->u.write_ctrlreg.new_value);
        break;

    case VM_EVENT_REASON_EMUL_UNIMPLEMENTED:
        xtf_log(lvl, "Emulation unimplemented: rip=%016lx, vcpu %d:\n",
            req->data.regs.x86.rip,
            req->vcpu_id);
        break;
    }
}

static void xtf_vm_event_ring_get_request(xtf_vm_event_ring_t *evt, vm_event_request_t *req)
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

static void xtf_vm_event_ring_put_response(xtf_vm_event_ring_t *evt, vm_event_response_t *rsp)
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

static int xtf_monitor_loop()
{
    vm_event_request_t req;
    vm_event_response_t rsp;
    int rc;

    /*
     * NOTE: The test harness waits for this message to unpause
     * the monitored DOMU.
     */
    printf("Monitor initialization complete.\n");

    for (;;)
    {
        rc = xtf_monitor_wait_for_event(100);
        if ( rc < -1 )
        {
            XTF_MON_ERROR("Error getting event");
            return rc;
        }
        else if ( rc == 1 )
        {
            XTF_MON_INFO("Domain %d exited\n", monitor->domain_id);
            return 0;
        }

        while ( RING_HAS_UNCONSUMED_REQUESTS(&monitor->ring.back_ring) )
        {
            xtf_vm_event_ring_get_request(&monitor->ring, &req);

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

            xtf_monitor_dump_request(XTF_MON_LOG_LEVEL_DEBUG, &req);

            if ( req.reason >= VM_EVENT_REASON_MAX || !monitor->handlers[req.reason] )
                XTF_MON_ERROR("Unhandled request: reason = %d\n", req.reason);
            else
            {
                rc = monitor->handlers[req.reason](&req, &rsp);
                if (rc)
                    return rc;

                /* Put the response on the ring */
                xtf_vm_event_ring_put_response(&monitor->ring, &rsp);
            }
        }
        /* Tell Xen page is ready */
        rc = xenevtchn_notify(monitor->ring.xce_handle, monitor->ring.local_port);
        if ( rc )
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

    monitor->status = XTF_MON_STATUS_RUNNING;
    monitor->log_lvl = XTF_MON_LOG_LEVEL_ERROR;

    /* test specific setup sequence */
    rc = call_helper(monitor->ops.setup, argc, argv);
    if ( rc )
    {
        monitor->status = XTF_MON_STATUS_ERROR;
        goto e_exit;
    }

    monitor->xch = xc_interface_open(NULL, NULL, 0);
    if ( !monitor->xch )
    {
        XTF_MON_FATAL("Error initialising xenaccess\n");
        rc = -EINVAL;
        monitor->status = XTF_MON_STATUS_ERROR;
        goto e_exit;
    }

    monitor->xsh = xs_open(XS_OPEN_READONLY);
    if ( !monitor->xsh )
    {
        XTF_MON_FATAL("Error opening XEN store\n");
        rc = -EINVAL;
        monitor->status = XTF_MON_STATUS_ERROR;
        goto cleanup;
    }

    if ( !xs_watch( monitor->xsh, "@releaseDomain", "RELEASE_TOKEN") )
    {
        XTF_MON_FATAL("Error monitoring releaseDomain\n");
        rc = -EINVAL;
        monitor->status = XTF_MON_STATUS_ERROR;
        goto cleanup;
    }

    /* test specific initialization sequence */
    rc = xtf_monitor_init();
    if ( !rc )
        rc = call_helper(monitor->ops.init);
    if ( rc )
    {
        monitor->status = XTF_MON_STATUS_ERROR;
        goto cleanup;
    }

    /* Run test */
    rc = xtf_monitor_loop();
    if ( rc )
    {
        XTF_MON_ERROR("Error running test\n");
        monitor->status = XTF_MON_STATUS_ERROR;
    }
    else
        monitor->status = call_helper(monitor->ops.get_result);

cleanup:
    /* test specific cleanup sequence */
    call_helper(monitor->ops.cleanup);
    xtf_monitor_cleanup();
    if ( monitor->xsh )
    {
        xs_unwatch(monitor->xsh, "@releaseDomain", "RELEASE_TOKEN");
        xs_close(monitor->xsh);
        monitor->xsh = NULL;
    }

    xc_interface_close(monitor->xch);

e_exit:
    xtf_print_status(monitor->status);
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
