#!/bin/csh
rm *.o
g++ -c -g -pg *.cpp
g++ -c -g -pg altMain.c
g++ -pg -o AltParser *.o
