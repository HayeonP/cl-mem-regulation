#ifndef IOCLT_COMMANDS_H
#define IOCLT_COMMANDS_H

#include <stdint.h>
#include "kbase_types.h"
#include "vinstr_types.h"

#define KBASE_IOCTL_TYPE 0x80

#define KBASE_IOCTL_VERSION_CHECK \
	_IOWR(KBASE_IOCTL_TYPE, 0, struct version_check)

#define KBASE_IOCTL_SET_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 1, struct set_flags)

#define KBASE_IOCTL_GET_GPUPROPS \
	_IOW(KBASE_IOCTL_TYPE, 3, struct get_gpuprops)

#define KBASE_IOCTL_HWCNT_READER_SETUP \
	_IOW(KBASE_IOCTL_TYPE, 8, struct hwcnt_reader_setup)


#define KBASE_HWCNT_READER 0xBE

#define KBASE_HWCNT_READER_GET_API_VERSION_WITH_FEATURES \
	_IOW(KBASE_HWCNT_READER, 0xFF, struct vinstr_reader_api_version)

#define KBASE_HWCNT_READER_GET_BUFFER_SIZE \
	_IOR(KBASE_HWCNT_READER, 0x01, uint32_t)

#define KBASE_HWCNT_READER_DUMP \
	_IOW(KBASE_HWCNT_READER, 0x10, uint32_t)

#define KBASE_HWCNT_READER_CLEAR \
	_IOW(KBASE_HWCNT_READER, 0x11, uint32_t)


#define KBASE_HWCNT_READER_GET_BUFFER \
	_IOR(KBASE_HWCNT_READER, 0x20, struct vinstr_reader_metadata)

#define KBASE_HWCNT_READER_GET_BUFFER_WITH_CYCLES \
	_IOR(KBASE_HWCNT_READER, 0x20, struct vinstr_reader_metadata_with_cycles)

#define KBASE_HWCNT_READER_PUT_BUFFER \
	_IOR(KBASE_HWCNT_READER, 0x21, struct vinstr_reader_metadata)


#endif