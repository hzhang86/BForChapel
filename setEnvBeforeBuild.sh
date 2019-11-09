#!/bin/bash

source ./util/setchplenv.bash

export CHPL_TARGET_ARCH=native
# for multi-locale 
export CHPL_COMM=gasnet
export CHPL_COMM_SUBSTRATE=ibv

# launch options for ibv conduit, only one of them shall be enabled
# (3)
#export CHPL_LAUNCHER=slurm-gasnetrun_ibv
#export CHPL_LAUNCHER_WALLTIME=00:15:00
#export SLURM_PARTITION=debug
# (2)
export CHPL_LAUNCHER=gasnetrun_ibv
export GASNET_IBV_SPAWNER=ssh
# (1) is the same as (2)

# choose diff runtime: BForChapel needs
export CHPL_TASKS=fifo 
export CHPL_DEVELOPER=1

# llvm related setting
export CHPL_LLVM=llvm
export CHPL_LLVM_DEVELOPER=1
#export CHPL_WIDE_POINTERS=node16 #optional but useful with --llvm-wide-opt, should be disabled for version>=1.15

# check the environment set
./util/printchplenv
