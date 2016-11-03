#!/bin/bash

echo "compiling monitor"
g++ -c monitor.cpp -I$DYNINST_INSTALL/include
echo "linking monitor"
g++ -o monitor monitor.o -L$DYNINST_INSTALL/lib -lpcontrol -lstackwalk -lsymtabAPI -lcommon

