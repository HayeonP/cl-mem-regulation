# Cluster-level memory access regulation module (kernel module)
obj-m := cl_mem_reg.o

# Kernel source directory
KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

# Include project-specific headers
EXTRA_CFLAGS += -I$(PWD)/include

# GPU profiler (user-level thread)
GPU_PROFILER := gpu_profiler
GPU_PROFILER_SRC := gpu_profiler.c

all: kernel_module gpu_profiler

init:
	make -C $(KDIR) modules

# Build cl_mem_reg
kernel_module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Build GPU profiler
gpu_profiler:
	$(CC) -o $(GPU_PROFILER) $(GPU_PROFILER_SRC) -I./include

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) $(USER_APP)