#ifndef ARM_GPU_SAMPLE_POLL_H
#define ARM_GPU_SAMPLE_POLL_H

// backend/device/src/device/hwcnt/sampler/poll.hpp

#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

static inline int poll_fd(int fd, int timeout, int* is_ready);
int wait_for_sample(int fd);
int check_ready_read(int fd, int *is_ready);

int poll_fd(int fd, int timeout, int* is_ready) {
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;

    const nfds_t num_fds = 1;
    int result = poll(&fds, num_fds, timeout);

    if (result == -1) {
        perror("Poll error");
        return errno;
    }

    *is_ready = (result == num_fds);
    return 0;
}

// Function to wait for a hardware counters sample
int wait_for_sample(int fd) {
    int ready = 0;
    const int wait_forever = -1;

    int err = poll_fd(fd, wait_forever, &ready);
    if (err != 0) {
        return err;
    }

    if (!ready) {
        return ETIMEDOUT;
    }

    return 0;
}

// Function to check if a sample is ready to be read
int check_ready_read(int fd, int *is_ready) {
    const int no_wait = 0;
    return poll_fd(fd, no_wait, is_ready);
}

#endif