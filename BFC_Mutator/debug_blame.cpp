#include <stdio.h>
#include <stdlib.h>
#include <libunwind.h>
#include <string>
#include <vector>
#include <unistd.h>

#include <sys/resource.h>

//#include "BPatch.h"
//#include "BPatch_function.h"
//#include "BPatch_point.h"

#include <blameFuncs.h>

using namespace std;
/*
void handler(int EventSet, void *address, int  overflow_vector, void *context)
{


  unw_cursor_t cursor;
  unw_word_t ip, sp;
  unw_context_t uc;
  int ret;


  char buffer[256];

  gethostname(buffer, 255);


  FILE * pFile;
  pFile = fopen (buffer,"a");
  if (pFile==NULL)
    {
      printf("File failed to be created\n");
      return;
    }

        unw_word_t reg_EAX, reg_EDX, reg_ECX, reg_EBX, reg_ESI, reg_EDI;

        //fprintf(pFile,"<----START %s %d %d %d %d %d %d %d %d \n",buffer,                      );

//        int * loopIndex = (int *) topAddr;
        //printf("Here\n");
  fprintf(pFile,"<----START %s \n", buffer );

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    fprintf (stderr,"unw_init_local failed!\n");

        int count = 0;


      ret = unw_step (&cursor);
//  do
while(ret>0)
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);

      fprintf (pFile, "%d 0x%012lx \t", count, (long) ip);
      count++;

      ret = unw_step (&cursor);
      if (ret < 0)
      {
        unw_get_reg (&cursor, UNW_REG_IP, &ip);
        fprintf (stderr, "FAILURE: unw_step() returned %d for ip=%lx\n", ret, (long) ip);
      }
    }
//  while (ret > 0);

  fprintf(pFile,"\n");

  fprintf(pFile,"---->END\n");

  fclose(pFile);

}

*/

/*
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
*/
void level4(int n4)
{
        int four=n4*0xffff;
        handler(0, NULL,0, NULL);
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
