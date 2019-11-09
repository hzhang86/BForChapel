#!/bin/bash

# for multi-locale 
export CHPL_COMM=gasnet
export CHPL_COMM_SUBSTRATE=ibv
# some good options that can speedup the running
export GASNET_PHYSMEM_MAX=1G
export GASNET_ROUTE_OUTPUT=0 #TOCHECK
export GASNET_SSH_OPTIONS="-o LogLevel=Error" #disable login banner into the output

# launch options for ibv conduit, only one of them shall be enabled
# (3)
#export CHPL_LAUNCHER=slurm-gasnetrun_ibv
#export CHPL_LAUNCHER_USE_SBATCH=true //if it's set, then sbatch is used to launch the job to the queue system
# The following setting could be in a sbatch script 
#export CHPL_LAUNCHER_WALLTIME=00:15:00
#export SALLOC_PARTITION=debug
#export SLURM_PARTITION=$SALLOC_PARTITION
#export CHPL_LAUNCHER_SLURM_OUTPUT_FILENAME=./slurm-job.output
# (2)
export CHPL_LAUNCHER=gasnetrun_ibv
export GASNET_IBV_SPAWNER=ssh
# (1) is the same as (2)

# enable monitor process under the launcher
#export CHPL_LAUNCHER_REAL_WRAPPER=monitor
