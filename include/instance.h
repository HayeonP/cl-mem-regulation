#ifndef INSTANCE_H
#define INSTANCE_H

// backend/device/include/device/instance.hpp
// backend/device/src/device/instance_impl.hpp

#include <stdint.h>
#include "debug.h"
#include "kbase_types.h"
#include "constants.h"
#include "product_id.h"
#include "handle.h"
#include "num_exec_engines.h"
#include "hwcnt/block_extents.h"
#include "kbase_types.h"
#include "kbase_version.h"
#include "backend_type.h"
#include "ioctl_commands.h"
#include "sys/ioctl.h"
#include "tools.h"
#include "construct_block_extents.h"
#include "data_structure/vector.h"
#include "data_structure/bitset.h"

extern int enable_debug_;

struct instance {
    struct constants constants_;
    struct block_extents block_extents_;
    struct kbase_version kbase_version_;
    enum backend_type backend_type_;
    // TODO: enum_info ei_;

    int valid_;
    int fd_; // device_fd
};

struct prop_reader {
    uint8_t* buffer_;
    unsigned char *data_;
    size_t size_;
};

struct prop_decoder {
    struct prop_reader reader_;
};

uint64_t get_warp_width(uint64_t raw_gpu_id);

void prop_reader__init(struct prop_reader* reader, struct get_gpuprops* get_props);
uint8_t prop_reader_le_read_bytes_uint8_t(struct prop_reader* reader);
uint16_t prop_reader_le_read_bytes_uint16_t(struct prop_reader* reader);
uint32_t prop_reader_le_read_bytes_uint32_t(struct prop_reader* reader);
uint64_t prop_reader_le_read_bytes_uint64_t(struct prop_reader* reader);

void prop_decoder__init(struct prop_decoder* decoder, struct get_gpuprops* get_props);
void prop_decoder__destroy(struct prop_decoder* decoder);
void prop_decoder__next(struct prop_decoder* decoder, uint8_t* prop_id, uint64_t* value);
void prop_decoder__run(struct prop_decoder* decoder, struct constants* dev_consts);
void prop_decoder__to_prop_metadata(uint32_t v, uint8_t* prop_id, uint8_t* prop_size);

int instance__version_check(struct instance* instance);
int is_version_set(struct version_check* version);
int instance__get_fw_version(struct instance* instance);
int instance__version_check_pre_r21();
int instance__version_check_post_r21(struct instance* instance, enum ioctl_iface_type iface_type);
void instance__props_post_r21(int* fd, struct get_gpuprops* get_props);
void instance__set_flags(struct instance* instance);
void instance__init_constants(struct instance* instance);
void instance__backend_type_discover(struct kbase_version* version, struct product_id* pid, struct bitset* result);
void instance__backend_type_probe(struct instance* instance);
void instance__create(struct instance* instance, struct handle* hndl);
void instance__init(struct instance* instance, int* fd);



uint64_t get_warp_width(uint64_t raw_gpu_id){
    static struct product_id product_id_g31;
    static struct product_id product_id_g51;
    static struct product_id product_id_g52;
    static struct product_id product_id_g57;
    static struct product_id product_id_g57_2;
    static struct product_id product_id_g68;
    static struct product_id product_id_g71;
    static struct product_id product_id_g72;
    static struct product_id product_id_g76;
    static struct product_id product_id_g77;
    static struct product_id product_id_g78;
    static struct product_id product_id_g78ae;
    static struct product_id product_id_g310;
    static struct product_id product_id_g510;
    static struct product_id product_id_g610;
    static struct product_id product_id_g710;
    static struct product_id product_id_g615;
    static struct product_id product_id_g715;

    product_id__init_with_major(&product_id_g31, 7, 3);
    product_id__init_with_major(&product_id_g51, 7, 0);
    product_id__init_with_major(&product_id_g52, 7, 2);
    product_id__init_with_major(&product_id_g57, 9, 1);
    product_id__init_with_major(&product_id_g57_2, 9, 3);
    product_id__init_with_major(&product_id_g68, 9, 4);
    product_id__init_with_major(&product_id_g71, 6, 0);
    product_id__init_with_major(&product_id_g72, 6, 1);
    product_id__init_with_major(&product_id_g76, 7, 1);
    product_id__init_with_major(&product_id_g77, 9, 0);
    product_id__init_with_major(&product_id_g78, 9, 2);
    product_id__init_with_major(&product_id_g78ae, 9, 5);
    product_id__init_with_major(&product_id_g310, 10, 4);
    product_id__init_with_major(&product_id_g510, 10, 3);
    product_id__init_with_major(&product_id_g610, 10, 7);
    product_id__init_with_major(&product_id_g710, 10, 2);
    product_id__init_with_major(&product_id_g615, 11, 3);
    product_id__init_with_major(&product_id_g715, 11, 2);


    struct product_id pid;
    product_id__from_raw_gpu_id(&pid, raw_gpu_id);

    /* Midgard does not support warps. */
    if(product_id__get_gpu_family(&pid) == gpu_family_midgard){
        return 1;
    }

    if(pid.product_id_ == product_id_g31.product_id_) return 4;
    else if(pid.product_id_ == product_id_g68.product_id_) return 4;
    else if(pid.product_id_ == product_id_g51.product_id_) return 4;
    else if(pid.product_id_ == product_id_g71.product_id_) return 4;
    else if(pid.product_id_ == product_id_g72.product_id_) return 4;
    else if(pid.product_id_ == product_id_g52.product_id_) return 8;
    else if(pid.product_id_ == product_id_g76.product_id_) return 8;
    else if(pid.product_id_ == product_id_g57.product_id_) return 16;
    else if(pid.product_id_ == product_id_g57_2.product_id_) return 16;
    else if(pid.product_id_ == product_id_g77.product_id_) return 16;
    else if(pid.product_id_ == product_id_g78.product_id_) return 16;
    else if(pid.product_id_ == product_id_g78ae.product_id_) return 16;
    else if(pid.product_id_ == product_id_g310.product_id_) return 16;
    else if(pid.product_id_ == product_id_g510.product_id_) return 16;
    else if(pid.product_id_ == product_id_g610.product_id_) return 16;
    else if(pid.product_id_ == product_id_g710.product_id_) return 16;
    else if(pid.product_id_ == product_id_g615.product_id_) return 16;
    else if(pid.product_id_ == product_id_g715.product_id_) return 16;

    /* Newer GPUs are likely to have wide wraps. */
    if(product_id__get_arch_major(&pid) > 11)
        return 16;

    return 0;
}

void prop_reader__init(struct prop_reader* reader, struct get_gpuprops* get_props){
    memset(reader, 0, sizeof(struct prop_reader));
    reader->buffer_ = (uint8_t*)malloc(get_props->size);
    reader->buffer_ = memcpy(reader->buffer_, get_props->buffer, get_props->size);
    reader->data_ = reader->buffer_;
    reader->size_ = get_props->size;
}

uint8_t prop_reader_le_read_bytes_uint8_t(struct prop_reader* reader){
    uint8_t ret;
    for(size_t b = 0U; b < sizeof(uint8_t); ++b)
        ret |= (uint8_t)reader->data_[b] << (8 * b);
    reader->data_ += sizeof(uint8_t);
    reader->size_ -= sizeof(uint8_t);

    return ret;
}

uint16_t prop_reader_le_read_bytes_uint16_t(struct prop_reader* reader){
    uint16_t ret;
    for(size_t b = 0U; b < sizeof(uint16_t); ++b)
        ret |= (uint16_t)reader->data_[b] << (8 * b);
    reader->data_ += sizeof(uint16_t);
    reader->size_ -= sizeof(uint16_t);

    return ret;    
}

uint32_t prop_reader_le_read_bytes_uint32_t(struct prop_reader* reader){
    uint32_t ret = 0;
    for(size_t b = 0U; b < sizeof(uint32_t); ++b)
        ret |= (uint32_t)reader->data_[b] << (8 * b);
    reader->data_ += sizeof(uint32_t);
    reader->size_ -= sizeof(uint32_t);

    return ret;
}

uint64_t prop_reader_le_read_bytes_uint64_t(struct prop_reader* reader) {
    uint64_t ret = 0;
    for (size_t b = 0; b < sizeof(uint64_t); ++b) {
        ret |= (uint64_t)reader->data_[b] << (8 * b);
    }
    reader->data_ += sizeof(uint64_t);
    reader->size_ -= sizeof(uint64_t);

    return ret;
}

void prop_decoder__destroy(struct prop_decoder* decoder){
    free(decoder->reader_.buffer_);
}

void prop_decoder__init(struct prop_decoder* decoder, struct get_gpuprops* get_props){
    memset(decoder, 0, sizeof(struct prop_decoder));
    prop_reader__init(&(decoder->reader_), get_props);
}

void prop_decoder__next(struct prop_decoder* decoder, uint8_t* prop_id, uint64_t* value){
        /* Swallow any bad data. */
#define ERROR_IF_READER_SIZE_LT(x)                              \
    do {                                                        \
        if (decoder->reader_.size_ < (x)) {                     \
            *prop_id = 0;                                       \
            *value = 0;                                         \
            return;                                             \
        }                                                       \
    } while (0)

    ERROR_IF_READER_SIZE_LT(sizeof(uint32_t));

    uint32_t v = prop_reader_le_read_bytes_uint32_t(&(decoder->reader_));
    uint8_t prop_size = 0 ;

    prop_decoder__to_prop_metadata(v, prop_id, &prop_size);
    
    switch (prop_size) {
    case gpuprop_uint8:
        ERROR_IF_READER_SIZE_LT(sizeof(uint8_t));
        *value = prop_reader_le_read_bytes_uint8_t(&(decoder->reader_));        
        break;
    case gpuprop_uint16:
        ERROR_IF_READER_SIZE_LT(sizeof(uint16_t));
        *value = prop_reader_le_read_bytes_uint16_t(&(decoder->reader_));
        break;
    case gpuprop_uint32:
        ERROR_IF_READER_SIZE_LT(sizeof(uint32_t));
        *value = prop_reader_le_read_bytes_uint32_t(&(decoder->reader_));
        break;
    case gpuprop_uint64:
        ERROR_IF_READER_SIZE_LT(sizeof(uint64_t));
        *value = prop_reader_le_read_bytes_uint64_t(&(decoder->reader_));
        break;
    }
#undef ERROR_IF_READER_SIZE_LT

    return;
}

void prop_decoder__run(struct prop_decoder* decoder, struct constants* dev_consts){
    uint64_t num_core_groups = 0;
    uint64_t core_mask[16];
    uint64_t raw_core_features_v;
    uint64_t raw_thread_features_v;

    memset(dev_consts, 0, sizeof(struct constants));

    while(decoder->reader_.size_ > 0){
        uint8_t id;
        uint64_t value;

        prop_decoder__next(decoder, &id, &value);
        switch(id){
            case raw_gpu_id:
                dev_consts->gpu_id = value;
                dev_consts->warp_width = get_warp_width(value);
                break;
            case l2_log2_cache_size:
                dev_consts->l2_slice_size = (1UL << value);
                break;
            case l2_num_l2_slices:
                dev_consts->num_l2_slices = value;
                break;
            case raw_l2_features:
                /* log2(bus width in bits) stored in top 8 bits of register. */
                dev_consts->axi_bus_width = 1UL << ((value & 0xFF000000) >> 24);
                break;
            case raw_core_features:
                raw_core_features_v = value;
                break;
            case coherency_num_core_groups:
                num_core_groups = value;
                break;
            case raw_thread_features:
                raw_thread_features_v = value;
                break;
            case coherency_group_0:
                core_mask[0] = value;
                break;
            case coherency_group_1:
                core_mask[1] = value;
                break;
            case coherency_group_2:
                core_mask[2] = value;
                break;
            case coherency_group_3:
                core_mask[3] = value;
                break;
            case coherency_group_4:
                core_mask[4] = value;
                break;
            case coherency_group_5:
                core_mask[5] = value;
                break;
            case coherency_group_6:
                core_mask[6] = value;
                break;
            case coherency_group_7:
                core_mask[7] = value;
                break;
            case coherency_group_8:
                core_mask[8] = value;
                break;
            case coherency_group_9:
                core_mask[9] = value;
                break;
            case coherency_group_10:
                core_mask[10] = value;
                break;
            case coherency_group_11:
                core_mask[11] = value;
                break;
            case coherency_group_12:
                core_mask[12] = value;
                break;
            case coherency_group_13:
                core_mask[13] = value;
                break;
            case coherency_group_14:
                core_mask[14] = value;
                break;
            case coherency_group_15:
                core_mask[15] = value;
                break;
            case minor_revision:
            case major_revision:
            default:
                break;
        }
    }

    for(uint64_t i = 0U; i < num_core_groups; ++i){
        dev_consts->shader_core_mask |= core_mask[i];
    }

    dev_consts->num_shader_cores = __builtin_popcount(dev_consts->shader_core_mask);

    dev_consts->tile_size = 16;

    struct get_num_exec_engines_args args;
    memset(&args, 0, sizeof(struct get_num_exec_engines_args));
    product_id__from_raw_gpu_id(&args.id, dev_consts->gpu_id);
    args.core_count = dev_consts->num_shader_cores;
    args.core_features = raw_core_features_v;
    args.thread_features = raw_thread_features_v;
    dev_consts->num_exec_engines = get_num_exec_engines(&args);

    return;
}

void prop_decoder__to_prop_metadata(uint32_t v, uint8_t* prop_id, uint8_t* prop_size){
        /* Property id/size encoding is:
         * +--------+----------+
         * | 31   2 | 1      0 |
         * +--------+----------+
         * | PropId | PropSize |
         * +--------+----------+
         */
    static unsigned int prop_id_shift = 2;
    static unsigned int prop_size_mask = 0x3;

    *prop_id = (uint8_t)(v >> prop_id_shift);
    *prop_size = (uint8_t)(v & prop_size_mask);

    return;
}

int instance__version_check(struct instance* instance){
    /* TODO */
    // if (!instance__version_check_pre_r21()){
    //     return 0;
    // }

    if (!instance__version_check_post_r21(instance, ioctl_iface_type_jm_post_r21)){
        return 0;
    }


    // TODO
    return 0;
}

int is_version_set(struct version_check* version){
    return version->major != 0 || version->minor != 0;
}

int instance__get_fw_version(struct instance* instance){
    if(instance->kbase_version_.type_ != ioctl_iface_type_csf) return 0;

    // union cs_get_glb_iface get_glb;


    return 0;
}


int instance__version_check_pre_r21(){
    // TODO
    return 0;
}

int instance__version_check_post_r21(struct instance* instance, enum ioctl_iface_type iface_type){
    int fd;
    int ret;
    struct version_check version_check_args;
    memset(&version_check_args, 0, sizeof(struct version_check));

    fd = instance->fd_;

    if(iface_type == ioctl_iface_type_csf){
        // TODO
    }
    else{
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_IOCTL_VERSION_CHECK\n");
#endif
        ret = ioctl(instance->fd_, KBASE_IOCTL_VERSION_CHECK, &version_check_args);
        if (ret < 0){
            perror("ioctl KBASE_IOCTL_VERSION_CHECK failed");
            close(fd);
            return -1;
        }
    }
#ifdef ENABLE_DEBUG
        printf("(version_check_post_r_21): major: %u minor: %u\n", (unsigned int)version_check_args.major, (unsigned int)version_check_args.minor);
#endif

    if(!is_version_set(&version_check_args)) return -1;

    struct kbase_version kbase_version;
    kbase_version__init_with_args(&kbase_version, version_check_args.major, version_check_args.minor, iface_type);
    instance->kbase_version_ = kbase_version;

    return 0;
}

/** Get the raw properties buffer as it's returned from the kernel. */
void instance__props_post_r21(int* fd, struct get_gpuprops* get_props){
    int ret = 0;
    
    memset(get_props, 0, sizeof(struct get_gpuprops));
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_IOCTL_GET_GPUPROPS 1\n");
#endif
    ret = ioctl(*fd, KBASE_IOCTL_GET_GPUPROPS, get_props);
    if (ret < 0){
        perror("1st ioctl KBASE_IOCTL_GET_GPUPROPS failed");
        close(*fd);
        exit(1);
    }

    get_props->size = ret;
    get_props->flags = 0;
    get_props->buffer = (uint8_t*)malloc(get_props->size);
    if (get_props->buffer == NULL) {
        printf("[ERROR] Memory allocation is failed\n");
        exit(1);
    }
    memset(get_props->buffer, 0, get_props->size);

#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_IOCTL_GET_GPUPROPS 2\n");
#endif
    ret = ioctl(*fd, KBASE_IOCTL_GET_GPUPROPS, get_props);
    if (ret < 0){
        perror("2nd ioctl KBASE_IOCTL_GET_GPUPROPS failed");
        close(*fd);
        exit(1);
    }

#ifdef ENABLE_DEBUG
    printf("(props_post_r21) ret: %d, size: %u, flags: %u\n", ret, (uint32_t)get_props->size, (uint32_t)get_props->flags);
#endif

    return;
}

void instance__set_flags(struct instance* instance){
    int ret;
    int system_monitor_flag_submit_disabled_bit = 1;
    int system_monitor_flag = 1U << system_monitor_flag_submit_disabled_bit;

    if(instance->kbase_version_.type_ == ioctl_iface_type_jm_pre_r21){
        // TODO
    }
    else{
        struct set_flags flags;
        flags.create_flags = system_monitor_flag;
#ifdef ENABLE_IOCTL_DEBUG
        printf("ioctl: KBASE_IOCTL_SET_FLAGS\n");
#endif
        ret = ioctl(instance->fd_, KBASE_IOCTL_SET_FLAGS, &flags);
        if (ret < 0){
            perror("ioctl KBASE_IOCTL_SET_FLAGS failed");
            close(instance->fd_);
            exit(1);
        }
    }
#ifdef ENABLE_DEBUG
    printf("(set_flags) ret: %d\n", ret);
#endif

    return;
}

void instance__init_constants(struct instance* instance){
#ifdef ENABLE_DEBUG
    printf("(init_constants()) kernel_version type: %d\n", (int)(instance->kbase_version_.type_));
#endif

    if(instance->kbase_version_.type_ == ioctl_iface_type_jm_pre_r21){
        // TODO
    }
    
    struct get_gpuprops get_props;
    struct prop_decoder decoder;
    instance__props_post_r21(&instance->fd_, &get_props);

    prop_decoder__init(&decoder, &get_props);
    prop_decoder__run(&decoder, &instance->constants_);

#ifdef ENABLE_DEBUG
    printf("(init_constants) gpu_id: %lu / fw_version: %lu / axi_bus_width: %lu / num_shader_cores: %lu / shader_core_mask: %lu / num_l2_slices: %lu / l2_slice_size:%lu / num_exec_engines: %lu / tile_size: %lu / warp_width: %lu\n", instance->constants_.gpu_id, instance->constants_.fw_version, instance->constants_.axi_bus_width, instance->constants_.num_shader_cores, instance->constants_.shader_core_mask, instance->constants_.num_l2_slices, instance->constants_.l2_slice_size, instance->constants_.num_exec_engines, instance->constants_.tile_size, instance->constants_.warp_width);
#endif

    prop_decoder__destroy(&decoder);
    get_gpuprops__destroy(&get_props);

    return;
}

void instance__backend_type_discover(struct kbase_version* version, struct product_id* pid, struct bitset* result){
    bitset__init(result, backend_type_last+1);
    
    // TODO: Need to check all availabilty and setting bit
    bitset__set_bit(result, backend_type_vinstr);
    
    return;
}

void instance__backend_type_probe(struct instance* instance){
    struct product_id pid;
    struct bitset available_types;
    product_id__from_raw_gpu_id(&pid, instance->constants_.gpu_id);
    instance__backend_type_discover(&instance->kbase_version_, &pid, &available_types);
    backend_type_select(&available_types, &instance->backend_type_);
#ifdef ENABLE_DEBUG
    printf("(backend_type_probe) backend_type: %d\n", (int)instance->backend_type_);
#endif
    return;
}

void instance__init_block_extents(struct instance* instance){
    struct product_id pid;
    product_id__from_raw_gpu_id(&pid, instance->constants_.gpu_id);

#ifdef ENABLE_DEBUG
    printf("(init_block_extents) constants_.gpu_id: %d\n", (int)instance->constants_.gpu_id);
    printf("(init_block_extents) pid=product_id: %u\n", pid.product_id_);
#endif

    memset(&instance->block_extents_, 0, sizeof(struct block_extents));
    switch(instance->backend_type_){
        case backend_type_vinstr:
        case backend_type_vinstr_pre_r21:            
            construct_block_extents(&pid, instance->constants_.num_l2_slices, instance->constants_.num_shader_cores, &instance->block_extents_);            
            break;
        case backend_type_kinstr_prfcnt:
        case backend_type_kinstr_prfcnt_wa:
        case backend_type_kinstr_prfcnt_bad:
            //TODO
            break;
        default:
            break;
    }

    return;
}

void instance__backend_type_fixup(struct instance* instance){
    // TODO
    return;
}

void instance__create(struct instance* instance, struct handle* hndl){
    instance__init(instance, &hndl->fd_);

    return;
}

void instance__init(struct instance* instance, int* fd){
    instance->fd_ = *fd;
    instance__version_check(instance);
    instance__set_flags(instance);
    instance__init_constants(instance);
    instance__backend_type_probe(instance);
    instance__init_block_extents(instance);
    instance__backend_type_fixup(instance);

    return;
}

#endif