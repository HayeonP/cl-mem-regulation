#!/bin/bash

# Kill the gpu porfilers for cl1 and cl2
python3 process_killer.py
sleep 1

# Kill cl_mem_reg
rmmod cl_mem_reg
rm /tmp/cl_mem_reg_config.txt
