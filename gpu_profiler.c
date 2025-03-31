#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include "debug.h"
#include "gpu.h"
#include "sampler.h"
#include "hwcpipe_counter.h"

#define NOT_STARTED -1
#define TERMINATED -2
#define DEBUG_LOG
#define CL_MEM_REG_FORMAT "/dev/cl_mem_reg_cl%d"
#define VINSTR 0

#define MALI_MP12 0
#define MALI_MP3_1 1
#define MALI_MP3_2 2

volatile sig_atomic_t interrupt_in_progress = 0;

static struct gpu gpu;
static struct sampler_config sc;
static struct sampler sampler;
static struct counter_sample sample;
enum hwcpipe_counter counter1 = MaliExtBusRdBt;
enum hwcpipe_counter counter2 = MaliExtBusWrBt;
static int fd = -1;

int target_cluster;

void cleanup();

void profile_gpu_bandwidth(int signal) {

    if (signal == SIGUSR1) {
        if (fd == NOT_STARTED) {
            /* Device file path */
            char cl_mem_reg_path[256];
            snprintf(cl_mem_reg_path, sizeof(cl_mem_reg_path), CL_MEM_REG_FORMAT, target_cluster);

            fd = open(cl_mem_reg_path, O_WRONLY);
            if (fd < 0) {
                perror("User - Failed to open cl_mem_reg device");
                return;
            }
        }
        else if (fd == TERMINATED) {
            return;
        }
        
        int rd_beats = 0;
        int wr_beats = 0;
        
        sampler__sample_now(&sampler);

        sampler__get_counter_value(&sampler, counter1, &sample);
        wr_beats = sample.value.uint64;

        sampler__get_counter_value(&sampler, counter2, &sample);
        rd_beats = sample.value.uint64;

        int gpu_beats[2];
        gpu_beats[0] = rd_beats;
        gpu_beats[1] = wr_beats;
        if (write(fd, gpu_beats, sizeof(int) * 2) != sizeof(int) * 2) {
            perror("User - Failed to write GPU beats");
            return;
        }
    }
    else if (signal == SIGINT || signal == SIGTERM) {
        printf("Received termination signal (SIGINT or SIGTERM)\n");

        if (fd >= 0 && fd != TERMINATED) {
            close(fd);
            fd = TERMINATED;
        }
        
        /* Set sysfs path */
        const char *sysfs_path;
        if (target_cluster == 1) {
            sysfs_path = "/sys/kernel/cl_mem_reg_cl1/stop_gpu_bandwidth_profiling";
        } else if (target_cluster == 2) {
            sysfs_path = "/sys/kernel/cl_mem_reg_cl2/stop_gpu_bandwidth_profiling";
        } else {
            fprintf(stderr, "Invalid target cluster: %d\n", target_cluster);
            cleanup();
            exit(EXIT_FAILURE);
        }
  
        FILE *sysfs_file = fopen(sysfs_path, "w");
        if (sysfs_file == NULL) {
            perror("Failed to open sysfs file");
        } else {
            if (fprintf(sysfs_file, "0\n") < 0) {
                perror("Failed to write to sysfs file");
            }
            fclose(sysfs_file);
        }
        
        cleanup();
        exit(EXIT_SUCCESS);
    }

    return;
}

void cleanup() { 
    if (fd >= 0 && fd != TERMINATED) {
        close(fd);
        fd = TERMINATED;
    }
}

int main(int argc, char* argv[]) {
    struct sched_param param;
    FILE *pid_file;
    int gpu_type;
    int pid;
    
    if (argc < 3) {
        fprintf(stderr, "Input argument need to be <target_cluster> <GPU_type>\n");
        exit(EXIT_FAILURE);
    }

    target_cluster = strtol(argv[1], NULL, 10);
    if (target_cluster != 1 && target_cluster != 2) {
        fprintf(stderr, "Wrong target cluster: %d\n", target_cluster);
        exit(EXIT_FAILURE);
    }

    gpu_type = strtol(argv[2], NULL, 10);
    if (gpu_type != -1 && gpu_type != MALI_MP12 && gpu_type != MALI_MP3_1 && gpu_type != MALI_MP3_2) {
        fprintf(stderr, "Wrong GPU type: %d\n", gpu_type);
        exit(EXIT_FAILURE);
    }

    printf("=== Cluster%d GPU profiler ===\n", target_cluster);
    printf("GPU_TYPE: %d\n", gpu_type);

    /* Priority */
    param.sched_priority = 50;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("Cannot set SCHED_FIFO");
        exit(EXIT_FAILURE);
    }
    
    pid = (int)getpid();

    if (target_cluster == 1) {
        pid_file = fopen("/tmp/gpu_profiler_pid_cl1", "w");
        if (!pid_file) {
            perror("Failed to open /tmp/gpu_profiler_pid_cl1");
            exit(EXIT_FAILURE);
        }
        fprintf(pid_file, "%d\n", pid);
        fclose(pid_file);
    }
    else if (target_cluster == 2) {
        pid_file = fopen("/tmp/gpu_profiler_pid_cl2", "w");
        if (!pid_file) {
            perror("Failed to open /tmp/gpu_profiler_pid_cl2");
            exit(EXIT_FAILURE);
        }
        fprintf(pid_file, "%d\n", pid);
        fclose(pid_file);
    }
    
    struct sigaction sa;

    sa.sa_handler = profile_gpu_bandwidth;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Cannot initialize sigaction for SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Cannot initialize sigaction for SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Cannot initialize sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }
    
    // Init GPU and sampler
    gpu__init(&gpu, gpu_type);
    sampler_config__init(&sc, &gpu);

    sampler_config__add_counter(&sc, counter1);
    sampler_config__add_counter(&sc, counter2);

    sampler__init(&sampler, &sc);

    sampler__start_sampling(&sampler);

    counter_sample__init(&sample);

    printf("User mode GPU BW profiler - PID: %d\n", (int)getpid());
    printf("===============\n\n");

    fd = NOT_STARTED;

    while(1) {
        pause();
    }

    cleanup();
    return 0;
}
