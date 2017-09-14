/*
 * Xen public hvmop interface
 */

#ifndef XEN_PUBLIC_HVM_HVM_OP_H
#define XEN_PUBLIC_HVM_HVM_OP_H

/* Get/set subcommands: extra argument == pointer to xen_hvm_param struct. */
#define HVMOP_set_param           0
#define HVMOP_get_param           1
struct xen_hvm_param {
    domid_t  domid;
    uint32_t index;
    uint64_t value;
};
typedef struct xen_hvm_param xen_hvm_param_t;

/* HVMOP_altp2m: perform altp2m state operations */
#define HVMOP_altp2m 25

#define HVMOP_ALTP2M_INTERFACE_VERSION 0x00000001

struct xen_hvm_altp2m_domain_state {
    /* IN or OUT variable on/off */
    uint8_t state;
};
typedef struct xen_hvm_altp2m_domain_state xen_hvm_altp2m_domain_state_t;

struct xen_hvm_altp2m_view {
    /* IN/OUT variable */
    uint16_t view;
    /* Create view only: default access type
     * NOTE: currently ignored */
    uint16_t hvmmem_default_access; /* xenmem_access_t */
};
typedef struct xen_hvm_altp2m_view xen_hvm_altp2m_view_t;

struct xen_hvm_altp2m_op {
    uint32_t version;   /* HVMOP_ALTP2M_INTERFACE_VERSION */
    uint32_t cmd;
/* Get/set the altp2m state for a domain */
#define HVMOP_altp2m_get_domain_state     1
#define HVMOP_altp2m_set_domain_state     2
/* Set the current VCPU to receive altp2m event notifications */
#define HVMOP_altp2m_vcpu_enable_notify   3
/* Create a new view */
#define HVMOP_altp2m_create_p2m           4
/* Destroy a view */
#define HVMOP_altp2m_destroy_p2m          5
/* Switch view for an entire domain */
#define HVMOP_altp2m_switch_p2m           6
/* Notify that a page of memory is to have specific access types */
#define HVMOP_altp2m_set_mem_access       7
/* Change a p2m entry to have a different gfn->mfn mapping */
#define HVMOP_altp2m_change_gfn           8
/* Set access for an array of pages */
#define HVMOP_altp2m_set_mem_access_multi 9
    domid_t domain;
    uint16_t pad1;
    uint32_t pad2;
    union {
        struct xen_hvm_altp2m_domain_state         domain_state;
#if 0
        struct xen_hvm_altp2m_vcpu_enable_notify   enable_notify;
#endif
        struct xen_hvm_altp2m_view                 view;
#if 0
        struct xen_hvm_altp2m_set_mem_access       set_mem_access;
        struct xen_hvm_altp2m_change_gfn           change_gfn;
        struct xen_hvm_altp2m_set_mem_access_multi set_mem_access_multi;
#endif
        uint8_t pad[64];
    } u;
};
typedef struct xen_hvm_altp2m_op xen_hvm_altp2m_op_t;
#endif /* XEN_PUBLIC_HVM_HVM_OP_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
