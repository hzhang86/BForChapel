#!/bin/bash
#SBATCH -t 0:15:0
#SBATCH --nodes=32
#SBATCH --exclusive
##SBATCH --partition=debug
#SBATCH --output=./job.output

export GASNET_SSH_SERVERS=`scontrol show hostnames`
export GASNET_BACKTRACE=1 # in case error happens
export CHPL_RT_NUM_THREADS_PER_LOCALE=200
echo "Start running multi-locale Chapel with ibv-conduit!"
#./hpl -nl 2 --n=500 --printArrays=false --printStats=true --useRandomSeed=false
ENABLE_TASK=1 ENABLE_COMM=1 ENABLE_SAMP=1 ./hpl -nl 32 --n=500 --printArrays=false --printStats=true --useRandomSeed=false
srun addParser hpl_real

