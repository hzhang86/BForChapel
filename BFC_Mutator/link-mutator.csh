#!/bin/csh
g++ -g -o NoInstWithBlame test1.o -L${DYNINST_ROOT}/i386-unknown-linux2.4/lib -L/hivehomes/rutar/papi-3.5.0/src -L/hivehomes/rutar/InstrumentBlame -lblame -lpapi -lsymtabAPI -lstackwalk
