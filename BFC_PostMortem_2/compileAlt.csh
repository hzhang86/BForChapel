#!/bin/csh
rm *.o
g++ -c -g -pg *.cpp -std=c++11
g++ -c -g -pg altMain.cpp -std=c++11
g++ -pg -o AltParser *.o -std=c++11
