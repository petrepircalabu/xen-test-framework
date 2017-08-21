/*
 * XTF Monitor interface
 */

#ifndef XTF_MONITOR_H
#define XTF_MONITOR_H

#include <inttypes.h>
#include <xenctrl.h>
#include <xenevtchn.h>
#include <xen/vm_event.h>

typedef struct xtf_evtchn_ops
{
    int (*mem_access_handler)(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
    int (*singlestep_handler)(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
    int (*emul_unimpl_handler)(domid_t domain_id, vm_event_request_t *req, vm_event_response_t *rsp);
} xtf_evtchn_ops_t;

/** XTF Event channel interface */
typedef struct xtf_evtchn
{
    xenevtchn_handle *xce_handle;           /**< Event channel handle */
    xenevtchn_port_or_error_t remote_port;  /**< Event channel remote port */
    evtchn_port_t local_port;               /**< Event channel local port */
    vm_event_back_ring_t back_ring;         /**< vm_event back ring */
    void *ring_page;                        /**< Shared ring page */
    xtf_evtchn_ops_t *ops;                  /**< Test specific event callbacks */
} xtf_evtchn_t;

int add_evtchn(xtf_evtchn_t *evt, domid_t domain_id);
xtf_evtchn_t *get_evtchn(domid_t domain_id);
#define evtchn(domain_id) ( get_evtchn(domain_id) )

/** XTF Monitor Driver */
typedef struct xtf_monitor
{
    xc_interface *xch;                      /**< Control interface */
    xtf_evtchn_t *evt;                      /**< Event channel list */
    int (*setup)(int, char*[]);             /**< Test specific setup */
    int (*init)(void);                      /**< Test specific initialization */
    int (*run)(void);                       /**< Test specific routine */
    int (*cleanup)(void);                   /**< Test specific cleanup */
} xtf_monitor_t;

xtf_monitor_t *get_monitor();
#define monitor ( get_monitor() )
#define xtf_xch ( monitor->xch )

#define call_helper(func, ... )         ( (func) ? func(__VA_ARGS__) : 0 )
#define xtf_monitor_setup(argc, argv)   ( call_helper(monitor->setup, argc, argv) )
#define xtf_monitor_init()              ( call_helper(monitor->init) )
#define xtf_monitor_run()               ( call_helper(monitor->run) )
#define xtf_monitor_cleanup()           ( call_helper(monitor->cleanup) )

int xtf_evtchn_init(domid_t domain_id);
int xtf_evtchn_cleanup(domid_t domain_id);
int xtf_evtchn_loop(domid_t domain_id);

extern const char monitor_test_help[];

void usage();

#define XTF_MONITOR(pname) \
extern xtf_monitor_t *xtf_monitor_instance = &pname;

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
