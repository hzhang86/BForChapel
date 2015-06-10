#!/bin/csh
g++ -g -Wall -fPIC -DSTACK_WALK -I/export/home/hzhang86/packages/dyninst-head/include -I/fs/mashie/hzhang86/myBlame/include -D_GNU_SOURCE -W -Wall -DUSE_COMPILER_TLS -o mutator mutator.c -L/export/home/hzhang86/packages/dyninst-head/lib -L/fs/mashie/hzhang86/myBlame/lib -L./perfmon2-libpfm4/lib -lpfm -ldyninstAPI -lsymtabAPI -lcommon -lparseAPI -linstructionAPI -ldwarf

# g++ -g -Wall -fPIC -DSTACK_WALK -I${DYNINST_ROOT}/include  -I${DYNINST_ROOT}/dyninst -I${DYNINST_ROOT}/dyninst/external -I${DYNINST_ROOT}/symtabAPI -I${DYNINST_ROOT}/dyninst/dynutil/h  -D_GNU_SOURCE -W -Wall -DUSE_COMPILER_TLS -L${DYNINST_ROOT}/i386-unknown-linux2.4/lib -ldyninstAPI -lsymtabAPI  -o mutator mutator.C
