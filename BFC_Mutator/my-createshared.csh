#!/bin/csh
#g++ -g -fxceptions -Wall -Wsign-compare -shared -W1,-soname,libblame.so.1 -o libblame.so blameFuncs.o -L/export/home/hzhang86/packages/dyninst-head/lib -L../../lib -lunwind -lunwind-x86_64 -ldl -lsymtabAPI -lstackwalk  -lpapi

g++ -g -fxceptions -Wall -Wsign-compare -shared -W1,-soname,libblame.so.1 -o libblame.so blameFuncs.o ./perfmon2-libpfm4/perf_examples/perf_util.o -L/export/home/hzhang86/BForChapel/BFC_Mutator/perfmon2-libpfm4/lib -L/export/home/hzhang86/packages/dyninst-head/lib -L/fs/mashie/hzhang86/myBlame/lib -lpfm -lunwind -lunwind-x86_64 -ldl -lsymtabAPI -lstackwalk
