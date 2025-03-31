#ifndef KBASE_PRE_R21_TYPES_H
#define KBASE_PRE_R21_TYPES_H

// backend/device/src/device/ioctl/r21/types.hpp

#include <stdint.h>

enum class r21_header_id {
    /** Version check. */
    r21_version_check = 0x0,
    /** Base Context Create Kernel Flags. */
    r21_create_kernel_flags = 0x2,
    /** Kbase Func UK Func ID. */
    r21_uk_func_id = 512,
    /** Kbase Func Hwcnt Reader Setup. */
    r21_hwcnt_reader_setup = r21_uk_func_id + 36,
    /** Kbase Func Dump. */
    r21_dump = r21_uk_func_id + 11,
    /** Kbase Func Clear. */
    r21_clear = r21_uk_func_id + 12,
    /** Kbase Func Get Props. */
    r21_get_props = r21_uk_func_id + 14,
    /** Kbase Func Set Flags. */
    r21_set_flags = r21_uk_func_id + 18,
};

/** Message header. */
union r21_uk_header {
    /** 32-bit number identifying the UK function to be called. */
    r21_header_id id;
    /** The int return code returned by the called UK function. */
    uint32_t ret;
    /** Used to ensure 64-bit alignment of this union. Do not remove. */
    uint64_t sizer;
};

/** Check version compatibility between kernel and userspace. */
struct version_check_args {
    /** Header. */
    r21_uk_header header;
    /** Major version number. */
    uint16_t major;
    /** Minor version number */
    uint16_t minor;
};

/** IOCTL parameters to set flags */
struct r21_set_flags_args {
    /** Header. */
    r21_uk_header header;
    /** Create Flags. */
    uint32_t create_flags;
    /** Padding. */
    uint32_t padding;
};


#endif