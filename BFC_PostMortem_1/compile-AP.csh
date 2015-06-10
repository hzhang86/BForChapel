#!/bin/csh
# g++ -c -g -I/hivehomes/rutar/boost_1_36_0/ -I${DYNINST_ROOT}/dyninst -I${DYNINST_ROOT}/dyninst/dyninstAPI/h -I${DYNINST_ROOT}/dyninst/external -I/${DYNINST_ROOT}/dyninst/instructionAPI/h -I${DYNINST_ROOT}/dyninst/symtabAPI/h -I${DYNINST_ROOT}/dyninst/dynutil/h addParser.C

#g++ -L${DYNINST_ROOT}/i386-unknown-linux2.4/lib -ldyninstAPI -lsymtabAPI -linstructionAPI -lcommon -ldl  -o addParser addParser.o

g++ -c -g -I/export/home/hzhang86/packages/dyninst-head/include -I/fs/mashie/hzhang86/myBlame/include addParser.c
g++ -L/export/home/hzhang86/packages/dyninst-head/lib -L/fs/mashie/hzhang86/myBlame/lib -ldyninstAPI -lsymtabAPI -linstructionAPI -lparseAPI -lcommon -ldl -ldwarf -o addParser addParser.o -ldyninstAPI -lsymtabAPI -linstructionAPI -lparseAPI -lcommon -ldl -ldwarf
