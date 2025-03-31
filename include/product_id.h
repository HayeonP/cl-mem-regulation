#ifndef PRODUCT_ID_H
#define PRODUCT_ID_H

// backend/device/include/device/product_id.hpp

#include <stdint.h>
#include <string.h>

enum gpu_family {
    /** Midgard GPU family. */
    gpu_family_midgard,
    /** Bifrost GPU Family. */
    gpu_family_bifrost,
    /** Valhall GPU Family. */
    gpu_family_valhall,
};

enum gpu_frontend {
    /** Job manager GPUs. */
    gpu_frontend_jm,
    /** Command stream front-end GPUs. */
    gpu_frontend_csf,
};

enum version_style {
    /** Legacy style: product id is 0x6956 */
    version_style_legacy_t60x,
    /** Legacy style: product id is an arbitrary constant < 0x1000. */
    version_style_legacy_txxx,
    /** product id is an arch and product major versions pair. */
    version_style_arch_product_major,
};

struct product_id{
    uint32_t product_id_mask_legacy;
    uint32_t product_id_mask_modern;
    uint32_t product_id_t60x;
    uint32_t arch_major_shift;
    uint32_t version_mask;
    uint32_t product_id_;
};

void product_id__from_raw_gpu_id(struct product_id* product_id, uint64_t gpu_id);
void product_id__init_with_raw_product_id(struct product_id* product_id, uint32_t raw_product_id);
uint32_t product_id__from_versions(struct product_id* product_id, uint32_t arch_major, uint32_t product_major);
void product_id__init_with_raw_product_id(struct product_id* product_id, uint32_t raw_product_id);
void product_id__init_with_major(struct product_id* product_id, uint32_t arch_major, uint32_t product_major);
enum version_style product_id__get_version_style(struct product_id* product_id, uint32_t raw_produdct_id);
uint32_t product_id__get_product_major(struct product_id* product_id);
uint32_t product_id__get_arch_major(struct product_id* product_id);
enum gpu_family product_id__get_gpu_family(struct product_id* product_id);
enum gpu_frontend get_gpu_frontend(struct product_id* product_id);
uint32_t product_id__from_raw_product_id(struct product_id* product_id, uint32_t raw_product_id);



void product_id__from_raw_gpu_id(struct product_id* product_id, uint64_t gpu_id) {
    uint64_t product_id_shift = 16;
    uint64_t product_id_mask = 0xFFFF;

    product_id__init_with_raw_product_id(product_id, (uint32_t)((gpu_id >> product_id_shift) & product_id_mask));

    return;
}

uint32_t product_id__from_versions(struct product_id* product_id, uint32_t arch_major, uint32_t product_major){
    return (arch_major << product_id->arch_major_shift) | product_major;
}

void product_id__init(struct product_id* product_id){
    memset(product_id, 0, sizeof(struct product_id));
    product_id->product_id_mask_legacy = 0xffffU;
    product_id->product_id_mask_modern = 0xF00fU;
    product_id->product_id_t60x = 0x6956U;
    product_id->arch_major_shift = 12UL;
    product_id->version_mask = 0xfU;

    return;
}

void product_id__init_with_raw_product_id(struct product_id* product_id, uint32_t raw_product_id){
    product_id__init(product_id);
    product_id->product_id_ = product_id__from_raw_product_id(product_id, raw_product_id);
}

void product_id__init_with_major(struct product_id* product_id, uint32_t arch_major, uint32_t product_major){
    product_id__init(product_id);
    product_id->product_id_ = product_id__from_versions(product_id, arch_major, product_major);
}

enum version_style product_id__get_version_style(struct product_id* product_id, uint32_t raw_product_id){
    if(raw_product_id == product_id->product_id_t60x)
        return version_style_legacy_t60x;
    else if(raw_product_id < 0x1000U)
        return version_style_legacy_txxx;
    else
        return version_style_arch_product_major;
}

uint32_t product_id__get_product_major(struct product_id* product_id){
    return product_id->product_id_ & product_id->version_mask;
}

uint32_t product_id__get_arch_major(struct product_id* product_id){
    return (product_id->product_id_ >> product_id->arch_major_shift) & product_id->version_mask;
}

enum gpu_family product_id__get_gpu_family(struct product_id* product_id){
    uint32_t valhall_arch_major = 9U;
    if(product_id__get_version_style(product_id, product_id->product_id_) != version_style_arch_product_major)
        return gpu_family_midgard;
    
    if(product_id__get_arch_major(product_id) < valhall_arch_major)
        return gpu_family_bifrost;
    
    return gpu_family_valhall;
}

enum gpu_frontend get_gpu_frontend(struct product_id* product_id){
    uint32_t csf_arch_major = 0xaU;
    if(product_id__get_version_style(product_id, product_id->product_id_) != version_style_arch_product_major)
        return gpu_frontend_jm;
    if(product_id__get_arch_major(product_id) < csf_arch_major)
        return gpu_frontend_jm;
    
    return gpu_frontend_csf;
}

uint32_t product_id__from_raw_product_id(struct product_id* product_id, uint32_t raw_product_id){
    switch(product_id__get_version_style(product_id, raw_product_id)) {
        case version_style_legacy_t60x:
        case version_style_legacy_txxx:
            return raw_product_id & product_id->product_id_mask_legacy;
        case version_style_arch_product_major:
        default:
            return raw_product_id & product_id->product_id_mask_modern;
    }
}


#endif