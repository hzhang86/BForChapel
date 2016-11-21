/* This file is not used currently
 * just keep it for future use
*/
#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>
#include <string.h>
#include "Instances.h"


bool isForkStarWrapper(std::string name)
{
  if (name == "fork_wrapper" || name == "fork_nb_wrapper" ||
      name == "fork_large_wrapper" || name == "fork_nb_large_wrapper")
    return true;
  else
    return false;
}

bool isTopMainFrame(std::string name)
{
  if (name == "chpl_user_main" || name == "chpl_gen_main")
    return true;
  else
    return false;
}

bool forkInfoMatch(StackFrame &sf, Instance &inst)
{
  if (sf.info.callerNode == inst.info.callerNode &&
      sf.info.calleeNode == inst.info.calleeNode &&
      sf.info.fid == inst.info.fid &&
      sf.info.fork_num == inst.info.fork_num)
    return true;
  else
    return false;
}

#endif //_UTIL_H
