# Cluster-Level Memory Access Regulation

## Build and Setup
1. Build `cl_mem_reg` (Cluster-level Memory Access Regulation).

    It will build not only 'cl_mem_reg' but also 'gpu_profiler' (used by cl_mem_reg)
    ```
    make
    ```
    
2. Setup parameter of `cl_mem_reg.conf`.
    ```
    # === Configuration for Cluster-level Memory Access Regulation ===

    # Memory bandwidth budgets (MB/s)
    # -1: Unlimited (default: 204800 MB/s)
    BANDWIDTH_BUDGET_CL1=7500   # Cluster 1 budget
    BANDWIDTH_BUDGET_CL2=5000   # Cluster 2 budget

    # GPU assignment to clusters
    # 0: MP12, 1: MP3, -1: disable
    GPU_CL1=0                   # Cluster 1 GPU
    GPU_CL2=1                   # Cluster 2 GPU

    # GPU profiling core
    GPU_PROFILING_CORE_CL1=3    # Cluster 1 GPU profiling core (range: 0 to 3)
    GPU_PROFILING_CORE_CL2=7    # Cluster 2 GPU profiling core (range: 4 to 7)

    # Regulation period and Aggregation period (microseconds)
    REGULATION_PERIOD=6600      # Regulation period (T_R): Interval for regulation reset
    AGGREGATION_PERIOD=200      # Aggregation period (T_A): Interval for aggregation and throttling

    # Logging memory access amounts for CPUs and GPUs at every T_A
    LOGGING=0                   # 1: enable, 0: disable
    ```

## Execute and terminate
1. Execute `cl_mem_reg`
    ```
    sh run_cl_mem_reg.sh
    ```

2. Terminate `cl_mem_reg`
    ```
    sh stop_cl_mem_reg.sh
    ```
---

## Acknowledgements
This project was inspired by and built upon the following work:

- Memguard: A per-core memory bandwidth reservation system for multi-core platforms. (https://github.com/heechul/memguard)

Our work extends the Hardware Counter (HWC)-based periodic memory profiling and throttling technique from Memguard, scaling it from a per-core basis to a per-cluster (CPU+GPU) level implementation. This enhancement allows for more efficient memory bandwidth management in complex multi-core and multi-GPU environments.
