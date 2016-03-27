#!/bin/bash

echo "compiling monitor"
#g++ -c monitor.cpp -I/export/home/hzhang86/packages/dyninst-9.1.0/include
g++ -c monitor.cpp -I/export/home/hzhang86/packages/DyninstAPI-8.1.2-install/include
echo "linking monitor"
#g++ -o monitor monitor.o -L/export/home/hzhang86/packages/dyninst-9.1.0/lib -lpcontrol -lstackwalk
g++ -o monitor monitor.o -L/export/home/hzhang86/packages/DyninstAPI-8.1.2-install/lib -lpcontrol -lstackwalk -lsymtabAPI

