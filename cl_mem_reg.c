// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2013  Heechul Yun <heechul@illinois.edu>
 *           (C) 2022  Heechul Yun <heechul.yun@ku.edu>
 *           (C) 2024  Hayeon Park <hypark@rubis.snu.ac.kr>
 *
 * This file is distributed under GPL v2 License. 
 * See LICENSE.TXT for details.
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**************************************************************************
 * Header files
 **************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/pid.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/sched/rt.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/irq_work.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/perf_event.h>
#include <linux/seq_file.h>
#include <linux/smp.h> /* IPI calls */
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/preempt.h>
#include <linux/mutex.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
#include <uapi/linux/sched/types.h>
#elif LINUX_VERSION_CODE > KERNEL_VERSION(4, 13, 0)
#include <linux/sched/types.h>
#endif
#include <linux/sched.h>


/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define DEBUG(x)
#define DEBUG_RECLAIM(x) x
#define DEBUG_USER(x)
#define DEBUG_PROFILE(x) x

/* PMU counters */
#if defined(__aarch64__) || defined(__arm__)
#define PMU_LLC_RD_COUNTER_ID 0x2A // ARMV8_PMUV3_PERFCTR_L3D_CACHE_REFILL
#elif defined(__x86_64__) || defined(__i386__)
#define PMU_LLC_RD_COUNTER_ID 0x08b0 // OFFCORE_REQUESTS.ALL_DATA_RD
#elif defined(__riscv)
/* Note: These performance counters are specific to the T-Head C910.
 		 They may not function correctly on other RISC-V designs. */
#  define PMU_LLC_MISS_COUNTER_ID 0x11   // LL_CACHE_READ_MISS
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0) // somewhere between 4.4-4.10
#define TM_NS(x) (x)
#else
#define TM_NS(x) (x).tv64
#endif

#define CACHE_LINE_SIZE 64 // B
#define BUF_SIZE 256
#define LOGGING_BUF_SIZE 128

#define DEFAULT_RD_BUDGET_MB 204800
#define NUMBER_OF_CORES 8
#define GPU_RD_BEATS_IDX 0
#define GPU_WR_BEATS_IDX 1

#define GPU_BEATS_CLEANED -1

#define US_TO_NS(us) us * 1000ULL

extern struct module __this_module;

/**************************************************************************
 * Public Types
 **************************************************************************/

static int module_unloading = 0; 

/* per CPU info */
struct core_info {
    int cpu;
    struct cluster_info *cluster_info;
    struct task_struct *throttled_task;
    ktime_t throttled_time;             // absolute time when throttled
    struct perf_event *read_event;      // PMC: LLC miss count

    /* throttle thread */
    struct task_struct *throttle_thread; // forced throttle idle thread
    wait_queue_head_t throttle_evt;      // throttle wait queue

    /* statistics */
    int64_t aggregation_period_cnt;      // active periods count
    s64 old_read_val;

    /* per-core hr timer */
    struct hrtimer hr_timer;

    /* Flag for gpu bandwidth profiling */
    int profile_gpu_bandwidth; 

    ktime_t start_time;

};

/* global info */
struct global_info {
    ktime_t start_time;
    ktime_t period_in_ktime;
    atomic64_t count;
};

/* cluster info */
struct cluster_info {
    char label[BUF_SIZE];

    /* cluster_info */
    int size; /* Number of cores in the cluster */
    uint64_t regulation_period_cnt;

    /* for control logic */
    int bandwidth_budget;
    int cur_bandwidth_budget;

    atomic_t bandwidth_usage;

    ktime_t throttled_time;

    int prev_read_throttle_error; /* check whether there was throttle error in
                                    the previous period for the read counter */

    int leader_core;

    cpumask_var_t cpu_mask;

    int is_throttled;
};

/* Logging work */
struct logging_work {
    struct work_struct work;
    struct file *file;
    char buf[LOGGING_BUF_SIZE];
};

/**************************************************************************
 * Function Prototypes
 **************************************************************************/
static void cl_mem_reg_on_each_cpu_mask(const struct cpumask *mask, smp_call_func_t func, void *info, bool wait);
static inline u64 convert_mb_to_events(int mb);
static inline int convert_events_to_mb(u64 events);
static inline void print_current_context(void);
static inline u64 perf_event_count(struct perf_event *event);
static inline u64 get_read_event_used(struct core_info *core_info);
static inline u64 get_cur_read_event(struct core_info *core_info);
static void __throttle_core(void *info);
static ssize_t cl_mem_reg_sysfs_show_cl1(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t cl_mem_reg_sysfs_show_cl2(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t cl_mem_reg_sysfs_store_cl1(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t cl_mem_reg_sysfs_store_cl2(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static enum hrtimer_restart timer_callback_master(struct hrtimer *timer);
static inline void regulation_period_func(void);
static inline void aggregation_period_func(void);
static inline void logging_work_func(struct work_struct* work);
static void timer_callback_slave(struct core_info *core_info);
static struct perf_event *init_counter(int cpu, int budget, int counter_id, void *callback);
static void __start_counter(void *info);
static void __stop_counter(void *info);
static void start_logging(void);
static void stop_logging(void);
static int cl_mem_reg_config_show(struct seq_file *m, void *v);
static int cl_mem_reg_config_open(struct inode *inode, struct file *filp);
static int cl_mem_reg_config_debugfs_init(void);
static int throttle_thread_func(void *arg);
static int gpu_beats_receiver_thread_func_cl1(void* arg);
static int gpu_beats_receiver_thread_func_cl2(void* arg);
static ssize_t gpu_bandwidth_profiling_write_cl1(struct file* file, const char __user *buf, size_t count, loff_t* ppos);
static ssize_t gpu_bandwidth_profiling_write_cl2(struct file* file, const char __user *buf, size_t count, loff_t* ppos);

/**************************************************************************
 * Global Variables
 **************************************************************************/
static int g_manage_period_interval = 0;
static int g_debug_cnt = 0;

static ktime_t g_hrtimer_start[8];
static ktime_t g_leader_hrtimer_start_cl1;
static ktime_t g_leader_hrtimer_start_cl2;

static struct cluster_info _cluster_info_cl1;
static struct cluster_info _cluster_info_cl2;

static struct dentry *cl_mem_reg_dir;
static struct global_info global_info;
static struct core_info __percpu *_core_info;

/* Param */
static int g_read_counter_id = PMU_LLC_RD_COUNTER_ID;
static int g_regulation_period_us = 5300;
static int g_aggregation_period_us = 100;
static int g_bandwidth_budget_cl1 = 204800;
static int g_bandwidth_budget_cl2 = 204800;
static int g_logging = 0;
static int g_gpu_profiling_cl1 = 1;
static int g_gpu_profiling_cl2 = 1;
static int g_gpu_profiling_core_cl1 = 3;
static int g_gpu_profiler_pid_cl1 = -1;
static int g_gpu_profiling_core_cl2 = 7;
static int g_gpu_profiler_pid_cl2 = -1;

/* Device file */
static int major_cl1, major_cl2;
static struct class* cl_mem_reg_class_cl1;
static struct class* cl_mem_reg_class_cl2;
static struct device* cl_mem_reg_device_cl1;
static struct device* cl_mem_reg_device_cl2;

/* GPU bandwidth profiling */
static void* gpu_beats_memory_cl1; // [0]: Read beats, [1]: Write beates
static void* gpu_beats_memory_cl2; // [0]: Read beats, [1]: Write beates
static int gpu_beats_ready_cl1 = 0;
static int gpu_beats_ready_cl2 = 0;
static struct task_struct* gpu_profiler_task_cl1 = NULL;
static struct task_struct* gpu_profiler_task_cl2 = NULL;
static struct kobject* kobj_cl1;
static struct kobject* kobj_cl2;
static DECLARE_WAIT_QUEUE_HEAD(gpu_beats_receiver_waitqueue_cl1);
static DECLARE_WAIT_QUEUE_HEAD(gpu_beats_receiver_waitqueue_cl2);

/* gpu beats receiver thread */
struct task_struct* gpu_beats_receiver_thread_cl1;
struct task_struct* gpu_beats_receiver_thread_cl2;
static struct workqueue_struct* g_logging_workqueue = NULL;

/* Logging */
static struct file* g_cpu_logging_files[NUMBER_OF_CORES];
static struct file* g_gpu_logging_file_cl1;
static struct file* g_gpu_logging_file_cl2;

/* Mutex */
static DEFINE_MUTEX(g_mutex_cl1);
static DEFINE_MUTEX(g_mutex_cl2);

/**************************************************************************
 * Module parameters
 **************************************************************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0)
module_param(g_read_counter_id, hexint, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#else
module_param(g_read_counter_id, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#endif

module_param(g_regulation_period_us, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(g_aggregation_period_us, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(g_bandwidth_budget_cl1, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(g_bandwidth_budget_cl2, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

module_param(g_gpu_profiling_core_cl1, int, 0644);
MODULE_PARM_DESC(g_gpu_profiling_core_cl1, "CPU core which propfiling cluster 1's GPU bandwidth");

module_param(g_gpu_profiler_pid_cl1, int, 0644);
MODULE_PARM_DESC(g_gpu_profiler_pid_cl1, "PID of gpu profiler of cluster 1 process");

module_param(g_gpu_profiling_core_cl2, int, 0644);
MODULE_PARM_DESC(g_gpu_profiling_core_cl2, "CPU core which propfiling cluster 2's GPU bandwidth");

module_param(g_gpu_profiler_pid_cl2, int, 0644);
MODULE_PARM_DESC(g_gpu_profiler_pid_cl2, "PID of gpu profiler of cluster 2 process");


module_param(g_logging, int, 0644);
MODULE_PARM_DESC(g_logging, "Logging flag");

static struct kobj_attribute cl_mem_reg_sysfs_attribute_cl1 =
    __ATTR(stop_gpu_bandwidth_profiling, 0660, cl_mem_reg_sysfs_show_cl1, cl_mem_reg_sysfs_store_cl1);

static struct kobj_attribute cl_mem_reg_sysfs_attribute_cl2 =
    __ATTR(stop_gpu_bandwidth_profiling, 0660, cl_mem_reg_sysfs_show_cl2, cl_mem_reg_sysfs_store_cl2);

static const struct file_operations cl_mem_reg_config_fops = {
    .open = cl_mem_reg_config_open,
    .write = NULL,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations gpu_bandwidth_profiling_fops_cl1 = {
    .owner = THIS_MODULE,
    .write = gpu_bandwidth_profiling_write_cl1,
};

static const struct file_operations gpu_bandwidth_profiling_fops_cl2 = {
    .owner = THIS_MODULE,
    .write = gpu_bandwidth_profiling_write_cl2,
};

/**************************************************************************
 * Local Functions
 **************************************************************************/
/* similar to on_each_cpu_mask(), but this must be called with IRQ disabled */
static void cl_mem_reg_on_each_cpu_mask(const struct cpumask *mask, smp_call_func_t func, void *info, bool wait) {
    int cpu; 
    unsigned long irq_flags;    

    local_irq_save(irq_flags);
    cpu = smp_processor_id();

    /* Desc.: If the current cpu in the mask, it doesn't execute func for the current cpu. */
    smp_call_function_many(mask, func, info, wait);

    /* Desc.: Therefore, it executes func for the current cpu after excute it for all other cores. */
    if (cpumask_test_cpu(cpu, mask)) {
        func(info);
    }
    local_irq_restore(irq_flags);
}

/** convert MB/s to #of events (i.e., LLC miss counts) per period */
static inline u64 convert_mb_to_events(int mb) {
    return div64_u64((u64)mb * 1024 * 1024,
                            CACHE_LINE_SIZE * (1000000 / (u64)g_regulation_period_us));
}

static inline int convert_events_to_mb(u64 events) {
    u64 divisor = (u64)g_regulation_period_us * 1024 * 1024;
    int mb = div64_u64(events * CACHE_LINE_SIZE * 1000000 + (divisor - 1), divisor);
    return mb;
}

static inline int convert_bytes_to_events(u64 bytes) {
    return div64_u64(bytes, CACHE_LINE_SIZE);
}

static inline void print_current_context(void) {
    trace_printk("in_interrupt(%ld)(hard(%ld),softirq(%d)"
                 ",in_nmi(%d)),irqs_disabled(%d)\n",
                 in_interrupt(), in_irq(), (int)in_softirq(), (int)in_nmi(), (int)irqs_disabled());
}

/** read current counter value. */
static inline u64 perf_event_count(struct perf_event *event) { return local64_read(&event->count) + atomic64_read(&event->child_count); }
static inline u64 get_read_event_used(struct core_info *core_info) { return perf_event_count(core_info->read_event) - core_info->old_read_val; }
static inline u64 get_cur_read_event(struct core_info *core_info) { return perf_event_count(core_info->read_event); }

static void __throttle_core(void *info) {
    struct core_info *core_info = this_cpu_ptr(_core_info);
    ktime_t start;
    start = ktime_get();

    core_info->throttled_task = current;
    core_info->throttled_time = start;
    wake_up_interruptible(&core_info->throttle_evt);

    return;
}


static ssize_t cl_mem_reg_sysfs_show_cl1(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "Use echo 0 > /sys/kernel/cl_mem_reg_cl1/stop_gpu_bandwidth_profiling to stop the gpu bandwidth profiling\n");
}

static ssize_t cl_mem_reg_sysfs_show_cl2(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "Use echo 0 > /sys/kernel/cl_mem_reg_cl2/stop_gpu_bandwidth_profiling to stop the gpu bandwidth profiling\n");
}

static ssize_t cl_mem_reg_sysfs_store_cl1(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {

    if (buf[0] == '0') {
        gpu_profiler_task_cl1 = NULL;;
        
        printk(KERN_INFO "Disable GPU profiler\n");
    }
    return count;
}

static ssize_t cl_mem_reg_sysfs_store_cl2(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {

    if (buf[0] == '0') {
        gpu_profiler_task_cl2 = NULL;;
        
        printk(KERN_INFO "Disable GPU profiler\n");
    }
    return count;
}

static enum hrtimer_restart timer_callback_master(struct hrtimer *timer)
{
    struct global_info *global = &global_info;
    struct core_info *core_info = this_cpu_ptr(_core_info);
    int cpu;
    int orun;

    ktime_t next_time;

    cpu = core_info->cpu;    

    /* must be irq disabled. hard irq */
    BUG_ON(!irqs_disabled());
    WARN_ON_ONCE(!in_interrupt());

    /* Stop counter (disable overflow event) */
    core_info->read_event->pmu->stop(core_info->read_event, PERF_EF_UPDATE);

    atomic64_cmpxchg(&global->count, core_info->aggregation_period_cnt, core_info->aggregation_period_cnt+1);

    core_info->aggregation_period_cnt = atomic64_read(&global->count);

    next_time = ktime_add_ns(core_info->start_time, core_info->aggregation_period_cnt * g_aggregation_period_us * 1000);

    hrtimer_start(timer, next_time, HRTIMER_MODE_ABS_PINNED);

    /* Assign local period */
    timer_callback_slave(core_info);

    /* Restart counter */
    core_info->read_event->pmu->start(core_info->read_event, PERF_EF_RELOAD);

    /* Regulation period handling */
    if (core_info->aggregation_period_cnt % g_manage_period_interval == 1) {
        regulation_period_func();
    }
    else{
        /* Aggregation period handling */
        aggregation_period_func();
    }

    return HRTIMER_NORESTART;
}

static inline void regulation_period_func(void) {
    struct core_info *core_info = this_cpu_ptr(_core_info);
    struct cluster_info *cluster_info = core_info->cluster_info;    

    /* Unthrottle core */
    core_info->throttled_task = NULL;
    cluster_info->is_throttled = 0;

    /* Reset usage if current CPU is the cluster leader */
    if (core_info->cpu == cluster_info->leader_core) {
        cluster_info->bandwidth_usage = (atomic_t) {(0)};
        cluster_info->regulation_period_cnt++;
    }

    return;
}

static void aggregation_period_func(void) {
    struct core_info *core_info = this_cpu_ptr(_core_info);
    struct cluster_info *cluster_info = core_info->cluster_info;
    s64 cur_cpu_bandwidth_usage; // events
    size_t profiling_info_buf_size = 0;
    struct timespec64 time;
    struct task_struct* gpu_profiler_task = NULL;

    if(module_unloading) return;

    ktime_get_real_ts64(&time);
    
    /* Profiling CPU memory bandwidth usage */
    cur_cpu_bandwidth_usage = get_read_event_used(core_info);
    core_info->old_read_val += cur_cpu_bandwidth_usage;
    atomic_add(cur_cpu_bandwidth_usage, &cluster_info->bandwidth_usage);

    /* Profiling GPU memory bandwidth usage */
    if (core_info->profile_gpu_bandwidth) {

        if(core_info->cpu < 4) {
            gpu_profiler_task = gpu_profiler_task_cl1;
        }
        else{
            gpu_profiler_task = gpu_profiler_task_cl2;
        }

        if(gpu_profiler_task && pid_alive(gpu_profiler_task)) {
            /* Request GPU memory bandwidth profiling */
            send_sig(SIGUSR1, gpu_profiler_task, 0);
        }
    }

    /* Logging (CPU) */
    if(g_logging && g_cpu_logging_files[core_info->cpu]) {
        struct logging_work* logging_work_data;

        logging_work_data = kmalloc(sizeof(struct logging_work), GFP_ATOMIC);
        if(!logging_work_data) {
            pr_err("Failed to allocate memory for logging work");
            return;
        }

        INIT_WORK(&logging_work_data->work, logging_work_func);

        logging_work_data->file = g_cpu_logging_files[core_info->cpu];
        profiling_info_buf_size = snprintf(logging_work_data->buf, LOGGING_BUF_SIZE, "%llu,%lld.%09ld,%llu,%d\n", cluster_info->regulation_period_cnt, (long long)time.tv_sec, time.tv_nsec, cur_cpu_bandwidth_usage, cluster_info->is_throttled);
        profiling_info_buf_size = profiling_info_buf_size > LOGGING_BUF_SIZE ? LOGGING_BUF_SIZE : profiling_info_buf_size;
        
        queue_work_on(core_info->cpu, g_logging_workqueue, &logging_work_data->work);
    }


    /* Check if the core is throttled */
    if (core_info->throttled_task != NULL) {
        return;
    }


    /* Throttle core if the read usage exceeds the current budget */
    if (atomic_read(&cluster_info->bandwidth_usage) > cluster_info->cur_bandwidth_budget) {
        if (core_info->cpu == cluster_info->leader_core) cluster_info->is_throttled = 1;
        cl_mem_reg_on_each_cpu_mask(cluster_info->cpu_mask, __throttle_core, NULL, 0);
    }

    g_debug_cnt++;

    return;
}

static int gpu_beats_receiver_thread_func_cl1(void* arg) {
    int cpunr = (unsigned long)arg;
    struct sched_param param;
    struct cluster_info *cluster_info;
    int* gpu_beats = NULL;
    int cur_gpu_bandwidth_usage_bytes = 0;  // MB/s
    s64 cur_gpu_bandwidth_usage = 0;        // events
    int profiling_info_buf_size = 0;
    struct timespec64 time;

    /* Get proper cluster info */
    cluster_info = &_cluster_info_cl1;

    /* Set higher priority than throttle_thread */
    param.sched_priority = MAX_RT_PRIO - 1; 
    sched_setscheduler(current, SCHED_FIFO, &param);

    gpu_beats = (int*)gpu_beats_memory_cl1;

    while (!kthread_should_stop() && cpu_online(cpunr)) {
        /* Wait for GPU beats to be ready */
        wait_event_interruptible(gpu_beats_receiver_waitqueue_cl1, gpu_beats_ready_cl1 || kthread_should_stop());

        ktime_get_real_ts64(&time);

        /* Size of a GPU beat = 16 bytes */ 
        cur_gpu_bandwidth_usage_bytes = (gpu_beats[GPU_RD_BEATS_IDX] * 16 + gpu_beats[GPU_WR_BEATS_IDX] * 16);
        cur_gpu_bandwidth_usage = convert_bytes_to_events(cur_gpu_bandwidth_usage_bytes);
        atomic_add(cur_gpu_bandwidth_usage, &cluster_info->bandwidth_usage);

        /* Logging (GPU) */
        if(g_logging && g_gpu_logging_file_cl1) {
            struct logging_work* logging_work_data;

            logging_work_data = kmalloc(sizeof(struct logging_work), GFP_ATOMIC);
            if(!logging_work_data) {
                pr_err("Failed to allocate memory for logging work");
                return -1;
            }

            INIT_WORK(&logging_work_data->work, logging_work_func);
            logging_work_data->file = g_gpu_logging_file_cl1;
            profiling_info_buf_size = snprintf(logging_work_data->buf, LOGGING_BUF_SIZE, "%llu,%lld.%09ld,%llu,%llu,%llu,%d\n", cluster_info->regulation_period_cnt, (long long)time.tv_sec, time.tv_nsec, cur_gpu_bandwidth_usage, (uint64_t)gpu_beats[GPU_RD_BEATS_IDX], (uint64_t)gpu_beats[GPU_WR_BEATS_IDX], cluster_info->is_throttled);
            profiling_info_buf_size = profiling_info_buf_size > LOGGING_BUF_SIZE ? LOGGING_BUF_SIZE : profiling_info_buf_size;
            logging_work_data->buf[profiling_info_buf_size] = '\0';
            
            queue_work(g_logging_workqueue, &logging_work_data->work);
        }

        gpu_beats[GPU_RD_BEATS_IDX] = GPU_BEATS_CLEANED;
        gpu_beats[GPU_WR_BEATS_IDX] = GPU_BEATS_CLEANED;

        gpu_beats_ready_cl1 = 0;

        /* Throttle core if the read usage exceeds the current budget */
        if (atomic_read(&cluster_info->bandwidth_usage) > cluster_info->cur_bandwidth_budget) {
            cl_mem_reg_on_each_cpu_mask(cluster_info->cpu_mask, __throttle_core, NULL, 0);
        }
    }

    return 0;
}

static int gpu_beats_receiver_thread_func_cl2(void* arg) {
    int cpunr = (unsigned long)arg;
    struct sched_param param;
    struct cluster_info *cluster_info;
    int* gpu_beats = NULL;
    int cur_gpu_bandwidth_usage_bytes = 0;  // MB/s
    s64 cur_gpu_bandwidth_usage = 0;        // events
    int profiling_info_buf_size = 0;
    struct timespec64 time;

    /* Get proper cluster info */
    cluster_info = &_cluster_info_cl2;

    /* Set higher priority than throttle_thread */
    param.sched_priority = MAX_RT_PRIO - 1; 
    sched_setscheduler(current, SCHED_FIFO, &param);

    gpu_beats = (int*)gpu_beats_memory_cl2;

    while (!kthread_should_stop() && cpu_online(cpunr)) {
        /* Wait for GPU beats to be ready */
        wait_event_interruptible(gpu_beats_receiver_waitqueue_cl2, gpu_beats_ready_cl2 || kthread_should_stop());

        ktime_get_real_ts64(&time);

        /* Size of a GPU beat = 16 bytes */ 
        cur_gpu_bandwidth_usage_bytes = (gpu_beats[GPU_RD_BEATS_IDX] * 16 + gpu_beats[GPU_WR_BEATS_IDX] * 16);
        cur_gpu_bandwidth_usage = convert_bytes_to_events(cur_gpu_bandwidth_usage_bytes);
        atomic_add(cur_gpu_bandwidth_usage, &cluster_info->bandwidth_usage);

        /* Logging (GPU) */
        if(g_logging && g_gpu_logging_file_cl2) {
            struct logging_work* logging_work_data;

            logging_work_data = kmalloc(sizeof(struct logging_work), GFP_ATOMIC);
            if(!logging_work_data) {
                pr_err("Failed to allocate memory for logging work");
                return -1;
            }

            INIT_WORK(&logging_work_data->work, logging_work_func);
            logging_work_data->file = g_gpu_logging_file_cl2;
            profiling_info_buf_size = snprintf(logging_work_data->buf, LOGGING_BUF_SIZE, "%llu,%lld.%09ld,%llu,%llu,%llu,%d\n", cluster_info->regulation_period_cnt, (long long)time.tv_sec, time.tv_nsec, cur_gpu_bandwidth_usage, (uint64_t)gpu_beats[GPU_RD_BEATS_IDX], (uint64_t)gpu_beats[GPU_WR_BEATS_IDX], cluster_info->is_throttled);
            profiling_info_buf_size = profiling_info_buf_size > LOGGING_BUF_SIZE ? LOGGING_BUF_SIZE : profiling_info_buf_size;
            logging_work_data->buf[profiling_info_buf_size] = '\0';
            
            queue_work(g_logging_workqueue, &logging_work_data->work);
        }

        gpu_beats[GPU_RD_BEATS_IDX] = GPU_BEATS_CLEANED;
        gpu_beats[GPU_WR_BEATS_IDX] = GPU_BEATS_CLEANED;

        gpu_beats_ready_cl2 = 0;

        /* Throttle core if the read usage exceeds the current budget */
        if (atomic_read(&cluster_info->bandwidth_usage) > cluster_info->cur_bandwidth_budget) {
            cl_mem_reg_on_each_cpu_mask(cluster_info->cpu_mask, __throttle_core, NULL, 0);
        }
    }

    return 0;
}

static inline void logging_work_func(struct work_struct* work) {
    struct logging_work *logging_work_data = container_of(work, struct logging_work, work);
    loff_t pos = 0;
    mm_segment_t old_fs;

    if (!logging_work_data->file) {
        pr_debug("File pointer is NULL in logging_work_func");
        kfree(logging_work_data);
        return;
    }

    pos = vfs_llseek(logging_work_data->file, 0, SEEK_END);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    kernel_write(logging_work_data->file, logging_work_data->buf, strlen(logging_work_data->buf), &pos);

    set_fs(old_fs);

    kfree(logging_work_data);
}

static void timer_callback_slave(struct core_info *core_info) {
    /* setup an interrupt */
    local64_set(&core_info->read_event->hw.period_left, convert_mb_to_events(DEFAULT_RD_BUDGET_MB));
}

static struct perf_event *init_counter(int cpu, int budget, int counter_id, void *callback) {
    struct perf_event *event = NULL;
    struct perf_event_attr sched_perf_hw_attr = {
        .type = PERF_TYPE_RAW,
        .size = sizeof(struct perf_event_attr),
        .pinned = 1,
        .disabled = 1,
        .config = counter_id,
        .sample_period = budget,
        .exclude_kernel = 0,           // 1 mean, no kernel mode counting
        .exclude_hv = 0,               // don't count hypervisor
        .exclude_idle = 0,             // don't count when idle
        .exclude_host = 0,             // don't count in host
        .exclude_guest = 0,            // don't count in guest
        .exclude_callchain_kernel = 0, // exclude kernel callchains
        .exclude_callchain_user = 0,
    };

    /* Try to register using hardware perf events */
    event = perf_event_create_kernel_counter(&sched_perf_hw_attr, cpu, NULL, callback
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
                                             ,
                                             NULL
#endif
    );

    if (!event)
        return NULL;

    if (IS_ERR(event)) {
        /* vary the KERN level based on the returned errno */
        if (PTR_ERR(event) == -EOPNOTSUPP)
            pr_info("cpu%d. not supported\n", cpu);
        else if (PTR_ERR(event) == -ENOENT)
            pr_info("cpu%d. not h/w event\n", cpu);
        else
            pr_err("cpu%d. unable to create perf event: %ld\n", cpu, PTR_ERR(event));
        return NULL;
    }

    /* success path */
    pr_info("cpu%d enabled counter 0x%x\n", cpu, counter_id);

    return event;
}

static void __start_counter(void *info) {

    struct global_info *global = &global_info;
    struct core_info *core_info = this_cpu_ptr(_core_info);
    struct cluster_info *cluster_info;
    int cpu, min_cpu, max_cpu;
    
    cpu = smp_processor_id();
    cluster_info = core_info->cluster_info;
    core_info->start_time = global->start_time;

    if (!cpumask_test_cpu(cpu, cluster_info->cpu_mask)) {
        g_hrtimer_start[cpu] = KTIME_MAX;
        return;
    }

    if(cpu < 4) {
        cluster_info = &_cluster_info_cl1;
        min_cpu = 0;
        max_cpu = 3;
    }
    else{
        cluster_info = &_cluster_info_cl2;
        min_cpu = 4;
        max_cpu = 7;
    }
    

    BUG_ON(!core_info->read_event);

    /* initialize hr timer */
    hrtimer_init(&core_info->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_PINNED); // HRTIMER_MODE_REL_PINNED: Run timer on specific cpu core
    core_info->hr_timer.function = &timer_callback_master;

    /* start timer */
    hrtimer_start(&core_info->hr_timer, global->start_time, HRTIMER_MODE_ABS_PINNED);
    g_hrtimer_start[cpu] = ktime_get();
    for(cpu = min_cpu; cpu < max_cpu + 1; cpu++) {
        ktime_t* leader_hrtimer_start;

        if (!cpumask_test_cpu(core_info->cpu, cluster_info->cpu_mask)) {
            continue;
        }

        if(cpu < 4) {
            leader_hrtimer_start = &g_leader_hrtimer_start_cl1;
        }
        else{
            leader_hrtimer_start = &g_leader_hrtimer_start_cl2;
        }

        if (g_hrtimer_start[cpu] < *leader_hrtimer_start) {
            *leader_hrtimer_start = g_hrtimer_start[cpu];
            cluster_info->leader_core = cpu;
        }
    }

    pr_info("LEADER CORE: %d", cluster_info->leader_core);

    core_info->aggregation_period_cnt = 0;
    core_info->throttled_task = NULL;

    return;
}

static void __stop_counter(void *info) {

    struct core_info *core_info = this_cpu_ptr(_core_info);
    struct cluster_info *cluster_info;

    cluster_info = core_info->cluster_info;

    if (!cpumask_test_cpu(core_info->cpu, cluster_info->cpu_mask)) {
        return;
    }

    pr_info("stop counter - cpu: %d", core_info->cpu);

    cluster_info = core_info->cluster_info;

    BUG_ON(!core_info->read_event);

    /* stop the throttled thread */
    core_info->throttled_task = NULL;
    core_info->aggregation_period_cnt = -1;

    /* stop the perf counter */
    core_info->read_event->pmu->stop(core_info->read_event, PERF_EF_UPDATE);

    /* stop timer */
    hrtimer_cancel(&core_info->hr_timer);

    pr_info("##  End of stop counter");

    return;
}

static int cl_mem_reg_config_show(struct seq_file *m, void *v) {
    struct cluster_info *cluster_info_cl1 = &_cluster_info_cl1;
    struct cluster_info *cluster_info_cl2 = &_cluster_info_cl2;
    int i;
    seq_printf(m, "=== Cluster-level Memory Access Regulation Module ===\n");
    /* Cluster 1 info */
    seq_printf(m, " - Cluster1 budget (MB/s): %d\n", g_bandwidth_budget_cl1);
    seq_printf(m, " - Cluster1 leader core: %d\n", cluster_info_cl1->leader_core);
    for(i = 0; i < 4; i++) {
        seq_printf(m, "    - Core%d hrtimer start time (ns): %lld\n", i, (long long int)ktime_to_ns(g_hrtimer_start[i]));    
    }
    if(g_gpu_profiling_cl1) {
        seq_printf(m, " - Cluster 1 GPU profiling: Activated (GPU profiler at CPU %d)\n", g_gpu_profiling_core_cl1);
    }
    seq_printf(m, "\n");

    /* Cluster 2 info */
    seq_printf(m, " - Cluster2 budget (MB/s): %d\n", g_bandwidth_budget_cl2);
    seq_printf(m, " - Cluster2 leader core: %d\n", cluster_info_cl2->leader_core);
    for(i = 4; i < 8; i++) {
        seq_printf(m, "    - Core%d hrtimer start time (ns): %lld\n", i, (long long int)ktime_to_ns(g_hrtimer_start[i]));    
    }
    if(g_gpu_profiling_cl2) {
        seq_printf(m, " - Cluster2 GPU profiling: Activated (GPU profiler at CPU %d)\n", g_gpu_profiling_core_cl2);
    }
    seq_printf(m, "\n");

    /* T_R and T_A */
    seq_printf(m, " - Regulation period (us): %d\n", g_regulation_period_us);
    seq_printf(m, " - Aggregation period (us): %d\n", g_aggregation_period_us);
    seq_printf(m, "\n");

    /* Logging */
    if (g_logging) {
        seq_printf(m, " - Logging: enabled");
    }
    else{
        seq_printf(m, " - Logging: disabled");
    }
    seq_printf(m, "\n");
    
    seq_printf(m, "===============\n");

    return 0;
}

static int cl_mem_reg_config_open(struct inode *inode, struct file *filp) { return single_open(filp, cl_mem_reg_config_show, NULL); }

static int cl_mem_reg_config_debugfs_init(void) {
    cl_mem_reg_dir = debugfs_create_dir(THIS_MODULE->name, NULL);
    BUG_ON(!cl_mem_reg_dir);

    debugfs_create_file("config", 0444, cl_mem_reg_dir, NULL, &cl_mem_reg_config_fops);

    return 0;
}



static int throttle_thread_func(void *arg) {
    int cpunr = (unsigned long)arg;
    struct core_info *core_info = per_cpu_ptr(_core_info, cpunr);

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 9, 0)
    sched_set_fifo(current);
#else
    static const struct sched_param param = {
        .sched_priority = MAX_USER_RT_PRIO / 2,
    };

    sched_setscheduler(current, SCHED_FIFO, &param);
#endif

    while (!kthread_should_stop() && cpu_online(cpunr)) {

        DEBUG(trace_printk("wait an event\n"));

        /* Desc.: Wake up when throttled_task is registered */
        wait_event_interruptible(core_info->throttle_evt, core_info->throttled_task || kthread_should_stop());

        DEBUG(trace_printk("got an event\n"));

        if (kthread_should_stop())
            break;

#ifdef THROTTLE_TEST
        pr_info("Start throttle at CPU %d (period_id: %d)", smp_processor_id(), debug_period_id);
#endif

        while (core_info->throttled_task && !kthread_should_stop()) {
            smp_mb();    // Desc.: Prepare to use memory (To avoid memory collision between cores)
            cpu_relax(); // Desc.: Excute nop operation
        }
    }

    DEBUG(trace_printk("exit\n"));
    return 0;
}

static ssize_t gpu_bandwidth_profiling_write_cl1(struct file* file, const char __user *buf, size_t count, loff_t* ppos) {
    if (count > sizeof(double) * 2) {
        pr_info("[ERROR] count > shared_memory_size");
        return -EINVAL;
    }

    if (copy_from_user(gpu_beats_memory_cl1, buf, count)) {
        pr_info("[ERROR] copy_from_user");
        return -EFAULT;
    }

    gpu_beats_ready_cl1 = 1;
    wake_up_interruptible(&gpu_beats_receiver_waitqueue_cl1);

    return count;
}

static ssize_t gpu_bandwidth_profiling_write_cl2(struct file* file, const char __user *buf, size_t count, loff_t* ppos) {
    if (count > sizeof(double) * 2) {
        pr_info("[ERROR] count > shared_memory_size");
        return -EINVAL;
    }

    if (copy_from_user(gpu_beats_memory_cl2, buf, count)) {
        pr_info("[ERROR] copy_from_user");
        return -EFAULT;
    }

    gpu_beats_ready_cl2 = 1;
    wake_up_interruptible(&gpu_beats_receiver_waitqueue_cl2);

    return count;
}

static void start_logging(void) {
    int i;
    struct cluster_info* cluster_info_cl1 = &_cluster_info_cl1;
    struct cluster_info* cluster_info_cl2 = &_cluster_info_cl2;

    if(!g_logging) return;

    /* Logging (CPU) */
    for_each_online_cpu(i) {
        char file_name[256];

        g_cpu_logging_files[i] = NULL;

        if(!cpumask_test_cpu(i, cluster_info_cl1->cpu_mask) && !cpumask_test_cpu(i, cluster_info_cl2->cpu_mask)) {
            continue;
        }

        snprintf(file_name, sizeof(file_name), "/tmp/cl_mem_reg_log_cpu%d.csv", i);
        g_cpu_logging_files[i] = filp_open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (IS_ERR(g_cpu_logging_files[i])) {
            pr_err("Failed to open file: %ld", PTR_ERR(g_cpu_logging_files[i]));

            return;
        }
    }

    /* Logging (GPU) */
    if(g_gpu_profiling_cl1) {
        g_gpu_logging_file_cl1 = filp_open("/tmp/cl_mem_reg_log_gpu_cl1.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (IS_ERR(g_gpu_logging_file_cl1)) {
            pr_err("Failed to open file: %ld", PTR_ERR(g_gpu_logging_file_cl1));
    
            return;
        }
    }

    if(g_gpu_profiling_cl2) {
        g_gpu_logging_file_cl2 = filp_open("/tmp/cl_mem_reg_log_gpu_cl2.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (IS_ERR(g_gpu_logging_file_cl2)) {
            pr_err("Failed to open file: %ld", PTR_ERR(g_gpu_logging_file_cl2));
    
            return;
        }
    }

    return;
}

static void stop_logging(void) {
    int i;

    for_each_online_cpu(i) {
        struct core_info* core_info = per_cpu_ptr(_core_info, i);

        if(!cpumask_test_cpu(core_info->cpu, core_info->cluster_info->cpu_mask) || g_cpu_logging_files[i] == NULL) {
            continue;
        }

        filp_close(g_cpu_logging_files[i], NULL);
    }

    if(g_gpu_profiling_cl1)
        filp_close(g_gpu_logging_file_cl1, NULL);
    if(g_gpu_profiling_cl2)
        filp_close(g_gpu_logging_file_cl2, NULL);

    return;
}

int ceiling(int a, int b) {
    return (a + b - 1) / b;
}

/**************************************************************************
 * Main Module
 **************************************************************************/

static int __init cl_mem_reg_init(void) {
    int i;
    struct global_info *global = &global_info;
    struct cluster_info *cluster_info_cl1, *cluster_info_cl2;

    /* Check params */
    if( (g_gpu_profiling_core_cl1 < -1 || g_gpu_profiling_core_cl1 > NUMBER_OF_CORES)) {
        pr_err("Invalid g_gpu_profiling_core_cl1: %d", g_gpu_profiling_core_cl1);
        return -EINVAL;
    }
    else if(g_gpu_profiling_core_cl1 == -1) {
        g_gpu_profiling_cl1 = 0;
    }
    else{
        g_gpu_profiling_cl1 = 1;
    }

    if( (g_gpu_profiling_core_cl2 < -1 || g_gpu_profiling_core_cl2 > NUMBER_OF_CORES)) {
        pr_err("Invalid g_gpu_profiling_core_cl2: %d", g_gpu_profiling_core_cl2);
        return -EINVAL;
    }
    else if(g_gpu_profiling_core_cl2 == -1) {
        g_gpu_profiling_cl2 = 0;
    }
    else{
        g_gpu_profiling_cl2 = 1;
    }

    pr_info("g_gpu_profiling_cl1: %d", g_gpu_profiling_cl1);
    pr_info("g_gpu_profiling_cl2: %d", g_gpu_profiling_cl2);

    /* initialized global_info structure */
    memset(global, 0, sizeof(struct global_info));
    if (g_regulation_period_us < 0 || g_regulation_period_us > 1000000) {
        printk(KERN_INFO "Must be 0 < period < 1 sec\n");
        return -ENODEV;
    }

    g_manage_period_interval = ceiling(g_regulation_period_us, g_aggregation_period_us);

    for(i = 0; i < NUMBER_OF_CORES; i++)
        g_hrtimer_start[i] = KTIME_MAX;

    /* initialize clusters_info_list */
    cluster_info_cl1 = &_cluster_info_cl1;
    cluster_info_cl2 = &_cluster_info_cl2;
    memset(cluster_info_cl1, 0, sizeof(struct cluster_info));
    memset(cluster_info_cl2, 0, sizeof(struct cluster_info));
    zalloc_cpumask_var(&cluster_info_cl1->cpu_mask, GFP_NOWAIT);
    zalloc_cpumask_var(&cluster_info_cl2->cpu_mask, GFP_NOWAIT);

    /* Initialize cluster1 info */
    cluster_info_cl1->size = 0;
    cluster_info_cl1->regulation_period_cnt = 0;
    cluster_info_cl1->cur_bandwidth_budget = cluster_info_cl1->bandwidth_budget =
        convert_mb_to_events(g_bandwidth_budget_cl1);

    cluster_info_cl1->bandwidth_usage = (atomic_t) {(0)};

    cluster_info_cl1->throttled_time = ktime_set(0, 0);
    cluster_info_cl1->leader_core = -1;

    for(i = 0; i <4; i++) {
        cpumask_set_cpu(i, cluster_info_cl1->cpu_mask);
    }
    cluster_info_cl1->size = 4;
    cluster_info_cl1->is_throttled = 0;

    /* Initialize cluster2 info */
    cluster_info_cl2->size = 0;
    cluster_info_cl2->regulation_period_cnt = 0;
    cluster_info_cl2->cur_bandwidth_budget = cluster_info_cl2->bandwidth_budget =
        convert_mb_to_events(g_bandwidth_budget_cl2);

    cluster_info_cl2->bandwidth_usage = (atomic_t) {(0)};

    cluster_info_cl2->throttled_time = ktime_set(0, 0);
    cluster_info_cl2->leader_core = -1;

    for(i = 4; i <8; i++) {
        cpumask_set_cpu(i, cluster_info_cl2->cpu_mask);
    }
    cluster_info_cl2->size = 4;
    cluster_info_cl2->is_throttled = 0;

    global->period_in_ktime = ktime_set(0, g_aggregation_period_us * 1000);

    /* sysfs */
    if(g_gpu_profiling_cl1) {
        kobj_cl1 = kobject_create_and_add("cl_mem_reg_cl1", kernel_kobj);
        if(!kobj_cl1) {
            pr_err("Cannot create sysfs for cluster1.");
            return -ENOMEM;
        }

        if(sysfs_create_file(kobj_cl1, &cl_mem_reg_sysfs_attribute_cl1.attr)) {
            kobject_put(kobj_cl1);
            return -ENOMEM;
        }

        /* Find gpu profiler task */
        gpu_profiler_task_cl1 = pid_task(find_vpid(g_gpu_profiler_pid_cl1), PIDTYPE_PID);
        if (!gpu_profiler_task_cl1) {
            pr_err("Failed to find gpu profiler task of cluster 1 by PID %d", g_gpu_profiler_pid_cl1);
            return -ESRCH;
        }

        /* Init shared memory */
        gpu_beats_memory_cl1 = kmalloc(sizeof(double)*2, GFP_KERNEL);
        if (!gpu_beats_memory_cl1) {
            pr_err("Failed to allocate shared memory for cluster 1\n");
            return -ENOMEM;
        }

        /* Create cl_mem_reg device file */
        major_cl1 = register_chrdev(0, "cl_mem_reg_cl1", &gpu_bandwidth_profiling_fops_cl1);
        if (major_cl1 < 0) {
            pr_err("Failed to register cl_mem_reg cl1 device file: %d\n", major_cl1);
            return major_cl1;
        }
        printk(KERN_INFO "New device registered: /dev/cl_mem_reg_cl1 with major %d\n", major_cl1);

        cl_mem_reg_class_cl1 = class_create(THIS_MODULE, "cl_mem_reg_cl1");
        if (IS_ERR(cl_mem_reg_class_cl1)) {
            unregister_chrdev(major_cl1, "cl_mem_reg_cl1");
            pr_err("Failed to create class\n");
            kfree(gpu_beats_memory_cl1);
            kfree(gpu_beats_memory_cl2);
            return PTR_ERR(cl_mem_reg_class_cl1);
        }

        cl_mem_reg_device_cl1 = device_create(cl_mem_reg_class_cl1, NULL, MKDEV(major_cl1, 0), NULL, "cl_mem_reg_cl1");
        if (IS_ERR(cl_mem_reg_device_cl1)) {
            class_destroy(cl_mem_reg_class_cl1);
            unregister_chrdev(major_cl1, "cl_mem_reg_cl1");
            pr_err("Failed to create device\n");
            kfree(gpu_beats_memory_cl1);
            kfree(gpu_beats_memory_cl2);
            return PTR_ERR(cl_mem_reg_device_cl1);
        }
    }

    if(g_gpu_profiling_cl2) {
        kobj_cl2 = kobject_create_and_add("cl_mem_reg_cl2", kernel_kobj);
        if(!kobj_cl2) {
            pr_err("Cannot create sysfs for cluster2.");
            return -ENOMEM;
        }

        if(sysfs_create_file(kobj_cl2, &cl_mem_reg_sysfs_attribute_cl2.attr)) {
            kobject_put(kobj_cl2);
            return -ENOMEM;
        }

        /* Find gpu profiler task */
        gpu_profiler_task_cl2 = pid_task(find_vpid(g_gpu_profiler_pid_cl2), PIDTYPE_PID);
        if (!gpu_profiler_task_cl2) {
            pr_err("Failed to find gpu profiler task of cluster 2 by PID %d", g_gpu_profiler_pid_cl2);
            return -ESRCH;
        }

        /* Init shared memory */
        gpu_beats_memory_cl2 = kmalloc(sizeof(double)*2, GFP_KERNEL);
        if (!gpu_beats_memory_cl2) {
            pr_err("Failed to allocate shared memory for cluster 2\n");
            return -ENOMEM;
        }

        /* Create cl_mem_reg device file */
        major_cl2 = register_chrdev(0, "cl_mem_reg_cl2", &gpu_bandwidth_profiling_fops_cl2);
        if (major_cl2 < 0) {
            pr_err("Failed to register cl_mem_reg cl2 device file: %d\n", major_cl2);
            return major_cl2;
        }
        printk(KERN_INFO "New device registered: /dev/cl_mem_reg_cl2 with major %d\n", major_cl2);

        cl_mem_reg_class_cl2 = class_create(THIS_MODULE, "cl_mem_reg_cl2");
        if (IS_ERR(cl_mem_reg_class_cl2)) {
            unregister_chrdev(major_cl2, "cl_mem_reg_cl2");
            pr_err("Failed to create class\n");
            kfree(gpu_beats_memory_cl1);
            kfree(gpu_beats_memory_cl2);
            return PTR_ERR(cl_mem_reg_class_cl2);
        }

        cl_mem_reg_device_cl2 = device_create(cl_mem_reg_class_cl2, NULL, MKDEV(major_cl2, 0), NULL, "cl_mem_reg_cl2");
        if (IS_ERR(cl_mem_reg_device_cl2)) {
            class_destroy(cl_mem_reg_class_cl2);
            unregister_chrdev(major_cl2, "cl_mem_reg_cl2");
            pr_err("Failed to create device\n");
            kfree(gpu_beats_memory_cl1);
            kfree(gpu_beats_memory_cl2);
            return PTR_ERR(cl_mem_reg_device_cl2);
        }
    }

    /* Print intialization information*/
    pr_info("NR_CPUS: %d, online: %d\n", NR_CPUS, num_online_cpus());
    if (g_read_counter_id >= 0)
        pr_info("RAW HW READ COUNTER ID: 0x%x\n", g_read_counter_id);
    pr_info("HZ=%d, g_regulation_period_us=%d, g_aggregation_period_us=%d\n", HZ, g_regulation_period_us, g_aggregation_period_us);

    _core_info = alloc_percpu(struct core_info);

    for_each_online_cpu(i) {
        struct core_info *core_info = per_cpu_ptr(_core_info, i);

        /* initialize per-core data structure */
        memset(core_info, 0, sizeof(struct core_info));
        core_info->cpu = i;

        /* throttled task pointer */
        core_info->throttled_task = NULL;

        /* update local period information */
        core_info->aggregation_period_cnt = 0;

        if (!cpumask_test_cpu(core_info->cpu, cluster_info_cl1->cpu_mask) && !cpumask_test_cpu(core_info->cpu, cluster_info_cl2->cpu_mask)) {
            continue;
        }

        /* create performance counter without overflow */
        core_info->read_event = init_counter(i, INT_MAX, g_read_counter_id, NULL);

        /* initialize statistics */
        core_info->throttled_time = ktime_set(0, 0);

        /* create and wake-up throttle threads */
        init_waitqueue_head(&core_info->throttle_evt);

        core_info->throttle_thread = kthread_create_on_node(throttle_thread_func, (void *)((unsigned long)i), cpu_to_node(i), "kthrottle/%d", i);

        pr_info("# cpu: %d, throttle_thread's pid: %d", i, (int)(core_info->throttle_thread->pid));

        perf_event_enable(core_info->read_event);

        BUG_ON(IS_ERR(core_info->throttle_thread));
        kthread_bind(core_info->throttle_thread, i); // Bind thread to specific core i
        wake_up_process(core_info->throttle_thread); // Desc.: Put in to the wait queue (Change task state to the TASK_RUNNING)

        /* Setup GPU profiling */
        if(i == g_gpu_profiling_core_cl1 && g_gpu_profiling_cl1) {
            core_info->profile_gpu_bandwidth = 1;
            gpu_beats_receiver_thread_cl1 = kthread_create_on_node(gpu_beats_receiver_thread_func_cl1, (void *)((unsigned long)i), cpu_to_node(i), "kgpu_beats_receiver/%d", i);

            BUG_ON(IS_ERR(gpu_beats_receiver_thread_cl1));
            kthread_bind(gpu_beats_receiver_thread_cl1, i); // Bind thread to specific core i
            wake_up_process(gpu_beats_receiver_thread_cl1); // Desc.: Put in to the wait queue (Change task state to the TASK_RUNNING)

            pr_info("Activate GPU beats receiver on CPU %d", i);
        }
        else if(i == g_gpu_profiling_core_cl2 && g_gpu_profiling_cl2) {
            core_info->profile_gpu_bandwidth = 1;
            gpu_beats_receiver_thread_cl2 = kthread_create_on_node(gpu_beats_receiver_thread_func_cl2, (void *)((unsigned long)i), cpu_to_node(i), "kgpu_beats_receiver/%d", i);

            BUG_ON(IS_ERR(gpu_beats_receiver_thread_cl2));
            kthread_bind(gpu_beats_receiver_thread_cl2, i); // Bind thread to specific core i
            wake_up_process(gpu_beats_receiver_thread_cl2); // Desc.: Put in to the wait queue (Change task state to the TASK_RUNNING)

            pr_info("Activate GPU beats receiver on CPU %d", i);
        }
        else{
            core_info->profile_gpu_bandwidth = 0;
        }

        if(i < 4) {
            core_info->cluster_info = cluster_info_cl1;
        }
        else{
            core_info->cluster_info = cluster_info_cl2;
        }
    }

    /* Initialize workqueue for profiling */
    if(g_logging) {
        g_logging_workqueue = create_workqueue("cl_mem_reg_logging_workqueue");
        if(!g_logging_workqueue) {
            pr_err("Failed to create logging workqueue");
            kfree(gpu_beats_memory_cl1);
            kfree(gpu_beats_memory_cl2);
            return -ENOMEM;
        }
    }

    pr_info("Workqueue created successfully\n");

    cl_mem_reg_config_debugfs_init();

    g_leader_hrtimer_start_cl1 = KTIME_MAX;
    g_leader_hrtimer_start_cl2 = KTIME_MAX;

    ktime_t now = ktime_get();
    ktime_t start_offset = ktime_set(1, 0); // Start offset: 1 sec
    global->start_time = ktime_add(now, start_offset);
    
    s64 time_ns = ktime_to_ns(global->start_time);

    pr_info("Current time: %lld.%09lld s\n", time_ns / 1000000000, time_ns % 1000000000);



    start_logging();
    on_each_cpu(__start_counter, NULL, 0);

    return 0;
}

static void __exit cl_mem_reg_exit(void)
{
    int i;
    struct cluster_info *cluster_info_cl1 = &_cluster_info_cl1;
    struct cluster_info *cluster_info_cl2 = &_cluster_info_cl2;

    pr_info("Start exit cl_mem_reg");

    /* Stop logging and set unloading flag */
    int close_log_file = 0;
    if (g_logging)
        close_log_file = 1;

    g_logging = 0;
    module_unloading = 1;

    /* Stop timers and performance event counters */
    on_each_cpu(__stop_counter, NULL, 1);
    pr_info("LLC bandwidth throttling disabled\n");

    /* Flush and destroy workqueue */
    if (g_logging_workqueue) {
        flush_workqueue(g_logging_workqueue);
        destroy_workqueue(g_logging_workqueue);
        g_logging_workqueue = NULL;
    }

    /* Terminate GPU profiling threads */
    if(g_gpu_profiling_cl1 && gpu_beats_receiver_thread_cl1) {
        gpu_beats_ready_cl1 = 1;
        wake_up_interruptible(&gpu_beats_receiver_waitqueue_cl1);
        kthread_stop(gpu_beats_receiver_thread_cl1);
        gpu_beats_receiver_thread_cl1 = NULL;
    }

    if(g_gpu_profiling_cl2 && gpu_beats_receiver_thread_cl2) {
        gpu_beats_ready_cl2 = 1;
        wake_up_interruptible(&gpu_beats_receiver_waitqueue_cl2);
        kthread_stop(gpu_beats_receiver_thread_cl2);
        gpu_beats_receiver_thread_cl2 = NULL;
    }

    /* Close log files */
    if (close_log_file)
        stop_logging();

    /* Cleanup remaining resources */
    for_each_online_cpu(i) {
        struct core_info *core_info = per_cpu_ptr(_core_info, i);
        
        if (!cpumask_test_cpu(i, cluster_info_cl1->cpu_mask) && !cpumask_test_cpu(i, cluster_info_cl2->cpu_mask)) {
            continue;
        }

        pr_info("Stopping kthrottle/%d\n", i);
        kthread_stop(core_info->throttle_thread);
        perf_event_disable(core_info->read_event);
        perf_event_release_kernel(core_info->read_event);
        core_info->read_event = NULL;
    }

    cluster_info_cl1 = NULL;
    cluster_info_cl2 = NULL;

    debugfs_remove_recursive(cl_mem_reg_dir);
    free_percpu(_core_info);

    /* Remove sysfs and device files */
    if(g_gpu_profiling_cl1) {
        if(kobj_cl1) {
            sysfs_remove_file(kobj_cl1, &cl_mem_reg_sysfs_attribute_cl1.attr);
            kobject_put(kobj_cl1);
            kobj_cl1 = NULL;
        }

        if (!IS_ERR(cl_mem_reg_device_cl1)) {
            device_destroy(cl_mem_reg_class_cl1, MKDEV(major_cl1, 0));
            cl_mem_reg_device_cl1 = NULL;
        }
        if (!IS_ERR(cl_mem_reg_class_cl1)) {
            class_destroy(cl_mem_reg_class_cl1);
            cl_mem_reg_class_cl1 = NULL;
        }
        unregister_chrdev(major_cl1, "cl_mem_reg_cl1");

        kfree(gpu_beats_memory_cl1);
    }

    if(g_gpu_profiling_cl2) {
        if(kobj_cl2) {
            sysfs_remove_file(kobj_cl2, &cl_mem_reg_sysfs_attribute_cl2.attr);
            kobject_put(kobj_cl2);
            kobj_cl2 = NULL;
        }

        if (!IS_ERR(cl_mem_reg_device_cl2)) {
            device_destroy(cl_mem_reg_class_cl2, MKDEV(major_cl2, 0));
            cl_mem_reg_device_cl2 = NULL;
        }
        if (!IS_ERR(cl_mem_reg_class_cl2)) {
            class_destroy(cl_mem_reg_class_cl2);
            cl_mem_reg_class_cl2 = NULL;
        }
        unregister_chrdev(major_cl2, "cl_mem_reg_cl2");

        kfree(gpu_beats_memory_cl2);
    }
}



module_init(cl_mem_reg_init);
module_exit(cl_mem_reg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hayeon Park");
MODULE_DESCRIPTION("cl_mem_reg: Cluster-level Memory Access Regulation Module");