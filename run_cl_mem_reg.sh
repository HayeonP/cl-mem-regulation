#!/bin/bash

# === Configuration for Cluster-level Memory Access Regulation Module ===
# Read parameters from 'cl_mem_reg.conf'
source ./cl_mem_reg.conf

# Default values
BANDWIDTH_BUDGET_CL1=${BANDWIDTH_BUDGET_CL1:--1}
BANDWIDTH_BUDGET_CL2=${BANDWIDTH_BUDGET_CL2:--1}
GPU_CL1=${GPU_CL1:-0}
GPU_CL2=${GPU_CL2:-1}
GPU_PROFILING_CORE_CL1=${GPU_PROFILING_CORE_CL1:-3}
GPU_PROFILING_CORE_CL2=${GPU_PROFILING_CORE_CL2:-7}
REGULATION_PERIOD=${REGULATION_PERIOD:-5300}
AGGREGATION_PERIOD=${AGGREGATION_PERIOD:-100}
LOGGING=${LOGGING:-1}
# ==================================================

# Convert budget of -1 to max value
if [ "$BANDWIDTH_BUDGET_CL1" -eq -1 ]; then
    BANDWIDTH_BUDGET_CL1=204800
fi

if [ "$BANDWIDTH_BUDGET_CL2" -eq -1 ]; then
    BANDWIDTH_BUDGET_CL2=204800
fi

# === GPU - Cluster 1 ===

if [ "$GPU_CL1" -eq 0 ] || [ "$GPU_CL1" -eq 1 ]; then
    TARGET_CLUSTER=1
    taskset -c $GPU_PROFILING_CORE_CL1 ./gpu_profiler $TARGET_CLUSTER $GPU_CL1 &
    sleep 1
elif [ "$GPU_CL1" -eq -1 ]; then
    GPU_PROFILING_CORE_CL1=-1
    GPU_PROFILER_PID_CL1=-1
else
    echo "Wrong GPU for cluster1. It should be 0, 1, or -1 (disable)"
    exit
fi

# === GPU - Cluster 2 ===

if [ "$GPU_CL2" -eq 0 ] || [ "$GPU_CL2" -eq 1 ]; then
    TARGET_CLUSTER=2
    taskset -c $GPU_PROFILING_CORE_CL2 ./gpu_profiler $TARGET_CLUSTER $GPU_CL2 &
    sleep 1
elif [ "$GPU_CL2" -eq -1 ]; then
    GPU_PROFILING_CORE_CL2=-1
    GPU_PROFILER_PID_CL2=-1
else
    echo "Wrong GPU for cluster2. It should be 0, 1, or -1 (disable)"
    exit
fi

echo 25 > /proc/sys/kernel/perf_cpu_time_max_percent
echo 100000 > /proc/sys/kernel/perf_event_max_sample_rate
echo 0 > /proc/sys/kernel/perf_cpu_time_max_percent

# Read the PID from the /tmp/gpu_profiler_pid_cl1 file
if [ "$GPU_PROFILING_CORE_CL1" -ge 0 ]; then    
    if [ -f /tmp/gpu_profiler_pid_cl1 ]; then
        GPU_PROFILER_PID_CL1=$(cat /tmp/gpu_profiler_pid_cl1)
    else
        echo "Error: /tmp/gpu_profiler_pid_cl1 file not found."
        exit 1
    fi
fi

# Read the PID from the /tmp/gpu_profiler_pid_cl2 file
if [ "$GPU_PROFILING_CORE_CL2" -ge 0 ]; then    
    if [ -f /tmp/gpu_profiler_pid_cl2 ]; then
        GPU_PROFILER_PID_CL2=$(cat /tmp/gpu_profiler_pid_cl2)
    else
        echo "Error: /tmp/gpu_profiler_pid_cl2 file not found."
        exit 1
    fi
fi

# Remove remain log files
start=$(echo $TARGET_CORES | cut -d'-' -f1)
end=$(echo $TARGET_CORES | cut -d'-' -f2)
for ((i=start; i<=end; i++)); do
    file="/tmp/cl_mem_reg_log$i.csv"
    if [ -e "$file" ]; then
        rm "$file"
    fi
done

sleep 1

# Run cl_mem_reg kernel module
insmod cl_mem_reg.ko \
    g_bandwidth_budget_cl1=$BANDWIDTH_BUDGET_CL1 \
    g_bandwidth_budget_cl2=$BANDWIDTH_BUDGET_CL2 \
    g_gpu_profiling_core_cl1=$GPU_PROFILING_CORE_CL1 \
    g_gpu_profiling_core_cl2=$GPU_PROFILING_CORE_CL2 \
    g_gpu_profiler_pid_cl1=$GPU_PROFILER_PID_CL1 \
    g_gpu_profiler_pid_cl2=$GPU_PROFILER_PID_CL2 \
    g_regulation_period_us=$REGULATION_PERIOD \
    g_aggregation_period_us=$AGGREGATION_PERIOD \
    g_logging=$LOGGING

sleep 1

cat /sys/kernel/debug/cl_mem_reg/config
cat /sys/kernel/debug/cl_mem_reg/config > /tmp/cl_mem_reg_config.txt
