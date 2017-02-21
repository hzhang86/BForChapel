#!/bin/bash
rm *.o
g++ -c -g *.cpp -std=c++11
g++ -c -g altMain.cpp -std=c++11
g++ -g -o AltParser *.o -std=c++11
