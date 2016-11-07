#!/bin/bash

export DYNINSTAPI_RT_LIB=/export/home/hzhang86/packages/DyninstAPI-8.1.2-install/lib/libdyninstAPI_RT.so
echo "compiling monitor"
#g++ -c monitor.cpp -I/export/home/hzhang86/packages/dyninst-9.1.0/include
g++ -c monitor.cpp -I/export/home/hzhang86/packages/DyninstAPI-8.1.2-install/include -I/export/home/hzhang86/BForChapel/Dependencies/boost_1_54-install/include
#g++ -c monitor.cpp -I/export/home/hzhang86/packages/dyninst-head/include
echo "linking monitor"
#g++ -o monitor monitor.o -L/export/home/hzhang86/packages/dyninst-9.1.0/lib -lpcontrol -lstackwalk
g++ -o monitor monitor.o -L/export/home/hzhang86/packages/DyninstAPI-8.1.2-install/lib -lpcontrol -lstackwalk -lsymtabAPI
#g++ -o monitor monitor.o -L/export/home/hzhang86/packages/dyninst-head/lib -lpcontrol -lstackwalk -lsymtabAPI

