#!/bin/bash

DYNINSTAPI_RT_LIB=/export/home/hzhang86/packages/dyninst-head/lib/libdyninstAPI_RT.so
#g++ -c -g -I$DYNINST_INSTALL/include -I/export/home/hzhang86/BForChapel/Dependencies/boost_1_54-install/include addParser.cpp -std=gnu++11
g++ -c -g -I/export/home/hzhang86/packages/dyninst-head/include -I/export/home/hzhang86/BForChapel/Dependencies/boost_1_54-install/include addParser.cpp -std=gnu++11
echo "Finish compiling"
#g++ -L$DYNINST_INSTALL/lib -o addParser addParser.o -ldyninstAPI -std=gnu++11 #-lsymtabAPI -linstructionAPI -lparseAPI -lcommon -ldl -ldwarf
g++ -L/export/home/hzhang86/packages/dyninst-head/lib -o addParser addParser.o -ldyninstAPI -std=gnu++11 #-lsymtabAPI -linstructionAPI -lparseAPI -lcommon -ldl -ldwarf
echo "Finish linking"
