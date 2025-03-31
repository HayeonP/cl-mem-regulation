#ifndef HANDLE_H
#define HANDLE_H
//   HWCPipe/backend/device/include/device/handle.hpp
// + HWCPipe/backend/device/src/device/handle_impl.hpp

#include <stdint.h>

enum mode {
    /** The descriptor is closed at destruction time. */
    internal,
    /** the descriptor is kept open at destruction time. */
    external
};

struct handle {
    int fd_; // device_fd
    enum mode mode_;
};

int handle__create(struct handle* handle, uint32_t instance_number){    
    memset(handle, 0, sizeof(struct handle));

    /** handle */
    char device_path[32]; 
    int device_fd;
    sprintf(device_path, "/dev/mali%d", instance_number);
    printf("device path:%s\n", device_path);

    device_fd = open(device_path, O_RDWR);
    if (device_fd < 0) {
        perror("Failed to open the device");
        return -1;
    }    

    /** handle_impl */
    handle->fd_ = device_fd;
    handle->mode_ = internal;

    return 0;
}


#endif