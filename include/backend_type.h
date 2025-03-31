#ifndef BACKEND_TYPE_H
#define BACKEND_TYPE_H

// backend/device/src/device/hwcnt/backend_type.hpp

#include <string.h>
#include "product_id.h"
#include "kbase_version.h"
#include "data_structure/bitset.h"

/**
 * HWCNT back-end types bit-mask.
 *
 * The back-end are listed in a priority order (highest to lowest).
 */
enum backend_type{
    /** vinstr available. */
    backend_type_vinstr,
    /** vinstr pre r21 available. */
    backend_type_vinstr_pre_r21,
    /** kinstr_prfcnt available. */
    backend_type_kinstr_prfcnt,
    /** kinstr_prfcnt workaround available. */
    backend_type_kinstr_prfcnt_wa,
    /** kinstr_prfcnt bad available. */
    backend_type_kinstr_prfcnt_bad,
    /** Sentinel. */
    backend_type_last = backend_type_kinstr_prfcnt_bad,
    /** Wrong backend type */
    backend_type_error,
};

int is_gtux_or_later(struct product_id* pid);
enum backend_type backend_type_from_str(const char *str);
int is_vinstr_available(struct kbase_version* version, struct product_id* pid);
int is_kinstr_prfcnt_available(struct kbase_version* version);
int is_kinstr_prfcnt_bad_available(struct kbase_version* version);
inline void backend_type_select(struct bitset* available_types, enum backend_type* backend_type);


int is_gtux_or_later(struct product_id* pid){
    uint32_t arch_major_gtux = 11U;
    if(product_id__get_version_style(pid, pid->product_id_) == version_style_arch_product_major
        && product_id__get_arch_major(pid) >= arch_major_gtux){
        return 1;
    }

    return 0;
}

enum backend_type backend_type_from_str(const char *str){
    if (!strcmp(str, "vinstr"))
        return backend_type_vinstr;
    if (!strcmp(str, "vinstr_pre_r21"))
        return backend_type_vinstr_pre_r21;
    if (!strcmp(str, "kinstr_prfcnt"))
        return backend_type_kinstr_prfcnt;
    if (!strcmp(str, "kinstr_prfcnt_wa"))
        return backend_type_kinstr_prfcnt_wa;
    if (!strcmp(str, "kinstr_prfcnt_bad"))
        return backend_type_kinstr_prfcnt_bad;

    return backend_type_error;
}

int is_vinstr_available(struct kbase_version* version, struct product_id* pid){
    if(is_gtux_or_later(pid)) return 0;

    struct kbase_version jm_max_version;
    struct kbase_version csf_max_version;

    jm_max_version.major_ = 11;
    jm_max_version.minor_ = 40;
    jm_max_version.type_ = ioctl_iface_type_jm_post_r21;

    csf_max_version.major_ = 1;
    csf_max_version.minor_ = 21;
    csf_max_version.type_ = ioctl_iface_type_csf;

    switch(version->type_){
        case ioctl_iface_type_jm_pre_r21:
            return 1;
        case ioctl_iface_type_jm_post_r21:
            /* version < jm_max_version */
            return kbase_version__compare(version, &jm_max_version);
        case ioctl_iface_type_csf:
            /* version < csf_max_version */
            return kbase_version__compare(version, &csf_max_version);
    }

    __builtin_unreachable();
    return 0;
}

int is_kinstr_prfcnt_available(struct kbase_version* version) {
    // Furhter works
    return 0;
}

int is_kinstr_prfcnt_bad_available(struct kbase_version* version) {
    // Further works
    return 0;
}

void backend_type_select(struct bitset* available_types, enum backend_type* backend_type){    
    /** TODO: Hard coded*/
    *backend_type = backend_type_vinstr;
}

#endif