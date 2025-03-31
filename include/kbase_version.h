#ifndef KBASE_VERSION_H
#define KBASE_VERSION_H

// backend/device/src/device/kbase_version.hpp

#include <stdint.h>

enum ioctl_iface_type {
    /** Pre R21 release Job manager kernel. */
    ioctl_iface_type_jm_pre_r21,
    /** Post R21 release Job manager kernel. */
    ioctl_iface_type_jm_post_r21,
    /** CSF kernel. */
    ioctl_iface_type_csf,
};

struct kbase_version {
    uint16_t major_;
    uint16_t minor_;
    enum ioctl_iface_type type_;
};

void kbase_version__init(struct kbase_version* kv){
    kv->major_ = 0;
    kv->minor_ = 0;
    kv->type_ = ioctl_iface_type_csf;

    return;
}

void kbase_version__init_with_args(struct kbase_version* kv, uint16_t major, uint16_t minor, enum ioctl_iface_type type){
    kv->major_ = major;
    kv->minor_ = minor;
    kv->type_ = type;
    
    return;
}

int kbase_version__compare(struct kbase_version *lhs, struct kbase_version *rhs) {    
    if (lhs->major_ < rhs->major_) {
        return -1;
    } else if (lhs->major_ > rhs->major_) {
        return 1;
    } else {
        if (lhs->minor_ < rhs->minor_) {
            return -1;
        } else if (lhs->minor_ > rhs->minor_) {
            return 1;
        } else {
            return 0;
        }
    }
}

#endif