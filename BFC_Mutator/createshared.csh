#!/bin/csh
g++ -g -fexceptions -Wall -Wsign-compare -shared -W1,-soname,libblame.so.1 -o libblame.so blameFuncs_papi.o -L/export/home/hzhang86/packages/dyninst-head/lib -L/fs/mashie/hzhang86/myBlame/lib -lunwind -lunwind-x86_64 -ldl -lsymtabAPI -lstackwalk  -lpapi
#gcc -g -fexceptions -Wall -Wsign-compare -shared -W1,-soname,libblame.so.1 -o libblame.so blameFuncs.o -L. -L/hivehomes/rutar/libunwind-install/lib -lunwind -lunwind-x86 -lc -lgcc -ldl -lsymtabAPI -lstackwalk  -lpapi -lstdc++

