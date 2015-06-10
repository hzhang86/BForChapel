#!/bin/csh
#g++ -c -g -Wall -fPIC -DSTACK_WALK -I/export/home/hzhang86/packages/dyninst-head/include -I../../include -D_GNU_SOURCE -DNDEBUG -g -fexceptions -D_GNU_SOURCE -W -Wall -DHAVE_CONFIG -DUSE_COMPILER_TLS -DPAPI_PENTIUM4_FP_X87_SSE_DP -DPAPI_PENTIUM4_VEC_SSE -DPERFCTR26 -DPENTIUM4 blameFuncs.cpp

gcc -g -Wall -fPIC -I./perfmon2-libpfm4/perf_examples -I./perfmon2-libpfm4/include -DCONFIG_PFMLIB_DEBUG -DCONFIG_PFMLIB_OS_LINUX -I./perfmon2-libpfm4/perf_examples -D_GNU_SOURCE -pthread -c -o ./perfmon2-libpfm4/perf_examples/perf_util.o ./perfmon2-libpfm4/perf_examples/perf_util.c

g++ -c -g -Wall -fPIC -DSTACK_WALK -I/export/home/hzhang86/packages/dyninst-head/include -I/fs/mashie/hzhang86/myBlame/include -I./perfmon2-libpfm4/perf_examples -I./perfmon2-libpfm4/include -DCONFIG_PFMLIB_DEBUG -DCONFIG_PFMLIB_OS_LINUX -I./perfmon2-libpfm4/perf_examples -D_GNU_SOURCE -W -Wall -pthread blameFuncs.cpp
