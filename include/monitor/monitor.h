/*
 * XTF Monitor interface
 */

#ifndef XTF_MONITOR_H
#define XTF_MONITOR_H

#include <inttypes.h>
#include <xenctrl.h>
#include <xenevtchn.h>
#include <xen/vm_event.h>

typedef struct xtf_monitor_ops
{
} xtf_monitor_ops_t;

typedef struct xtf_monitor
{

} xtf_monitor_t;


/** XTF Event interface */
typedef struct xtf_vm_event
{
    domid_t domain_id;                      /**< Remote domain id*/
    xenevtchn_handle *xce_handle;           /**< Event channel handle */
    xenevtchn_port_or_error_t remote_port;  /**< Event channel remote port */
    evtchn_port_t local_port;               /**< Event channel local port */
    vm_event_back_ring_t back_ring;         /**< vm_event back ring */
    void *ring_page;                        /**< Shared ring page */
} xtf_vm_event_t;

void usage();

int monitor_init(xc_interface *xch, xtf_vm_event_t *event, domid_t domain_id);

int monitor_cleanup(xc_interface *xch, xtf_vm_event_t *event);

extern const char monitor_test_help[];

#define MONITOR_COMMON_OPTIONS_LONG \
    {"help",    no_argument,    0,  'h'}\

#define MONITOR_COMMON_OPTIONS_SHORT "h"

#define MONITOR_COMMON_OPTIONS_HANDLER \
    case 'h': \
        usage(); \
    exit(0); \
    break; \

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
