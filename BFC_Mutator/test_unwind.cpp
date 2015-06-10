#define UNW_LOCAL_ONLY
#include <libunwind.h>
//#include "../../include/libunwind-common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

void show_backtrace (void) {
  unw_cursor_t cursor; unw_context_t uc;
  unw_word_t ip, sp;

  unw_getcontext(&uc);
  unw_init_local(&cursor, &uc);
  while (unw_step(&cursor) > 0) {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);
    printf ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);
  }
}

void level4(int n4)
{
	int four=n4*0xffff;
	show_backtrace();
}

int level3(double n3)
{
	double dummy = n3/1000;
	int ret = 500;
	level4(ret);
	return ret;
}

int level2(int n2)
{
	int ret;
	ret = level3(double(n2))+10;
	return ret;
}

int level1(int n1)
{
	int ret;
	ret = level2(n1);
	return ret;
}

int main()
{
	int original = 0xff0011;
	int result = level1(original);
	
	return 0;
}	
