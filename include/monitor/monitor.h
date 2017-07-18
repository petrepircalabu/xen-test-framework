/*
 * XTF Monitor interface
 */

#ifndef XTF_MONITOR_H
#define XTF_MONITOR_H

#include <inttypes.h>
#include <xenctrl.h>
#include <xenevtchn.h>
#include <xenstore.h>
#include <xen/vm_event.h>

/**
 * The value was chosen to be greater than any VM_EVENT_REASON_*
 */
#define VM_EVENT_REASON_MAX 20

/* Should be in sync with "test_status" from common/report.c */
enum xtf_mon_status
{
    XTF_MON_STATUS_RUNNING,
    XTF_MON_STATUS_SUCCESS,
    XTF_MON_STATUS_SKIP,
    XTF_MON_STATUS_ERROR,
    XTF_MON_STATUS_FAILURE,
};

enum xtf_mon_log_level
{
    XTF_MON_LOG_LEVEL_FATAL,
    XTF_MON_LOG_LEVEL_ERROR,
    XTF_MON_LOG_LEVEL_WARNING,
    XTF_MON_LOG_LEVEL_INFO,
    XTF_MON_LOG_LEVEL_DEBUG,
    XTF_MON_LOG_LEVEL_TRACE,
};

void xtf_log(enum xtf_mon_log_level lvl, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

#define XTF_MON_FATAL(format...)    xtf_log(XTF_MON_LOG_LEVEL_FATAL,    format)
#define XTF_MON_ERROR(format...)    xtf_log(XTF_MON_LOG_LEVEL_ERROR,    format)
#define XTF_MON_WARNING(format...)  xtf_log(XTF_MON_LOG_LEVEL_WARNING,  format)
#define XTF_MON_INFO(format...)     xtf_log(XTF_MON_LOG_LEVEL_INFO,     format)
#define XTF_MON_DEBUG(format...)    xtf_log(XTF_MON_LOG_LEVEL_DEBUG,    format)
#define XTF_MON_TRACE(format...)    xtf_log(XTF_MON_LOG_LEVEL_TRACE,    format)

typedef int (*vm_event_handler_t)(vm_event_request_t *, vm_event_response_t *);

/** XTF VM Event Ring interface */
typedef struct xtf_vm_event_ring
{
    /* Event channel handle */
    xenevtchn_handle *xce_handle;

    /* Event channel remote port */
    xenevtchn_port_or_error_t remote_port;

    /* Event channel local port */
    evtchn_port_t local_port;

    /* vm_event back ring */
    vm_event_back_ring_t back_ring;

    /* Shared ring page */
    void *ring_page;
} xtf_vm_event_ring_t;

typedef struct xtf_monitor_ops
{
    /* Test specific setup */
    int (*setup)(int, char*[]);

    /* Test specific initialization */
    int (*init)(void);

    /* Test specific cleanup */
    int (*cleanup)(void);

    /* Returns the test's result */
    int (*get_result)(void);
} xtf_monitor_ops_t;

/* XTF Monitor Driver */
typedef struct xtf_monitor
{
    /* Domain ID */
    domid_t domain_id;

    /* LibXC intreface handle */
    xc_interface *xch;

    /* XEN store handle */
    struct xs_handle *xsh;

    /* XTF VM_EVENT ring */
    xtf_vm_event_ring_t ring;

    /* Log Level */
    enum xtf_mon_log_level log_lvl;

    /* Test status */
    enum xtf_mon_status status;

    /* Test Help message*/
    const char * help_message;

    /* Test specific operations */
    xtf_monitor_ops_t ops;

    /* Test specific VM_EVENT request handlers */
    vm_event_handler_t handlers[VM_EVENT_REASON_MAX];

} xtf_monitor_t;

void usage();

extern xtf_monitor_t *monitor;

#define XTF_MONITOR(param) \
static void  __attribute__((constructor)) register_monitor_##param() \
{ \
    monitor = (xtf_monitor_t *)&param; \
}

#endif /* XTF_MONITOR_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
