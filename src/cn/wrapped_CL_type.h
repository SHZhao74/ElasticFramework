// Copyright [2013] (JoenChen)
#ifndef NEWSCL_CN_WARPPED_CL_TYPE_H_
#define NEWSCL_CN_WARPPED_CL_TYPE_H_
#include <memory>
#include <map>

#include <CL/cl.h>

#include "VM.h"

struct MemObject {
  cl_mem mem;
  cl_command_queue newest_on_dev;
  unsigned int size;
};

struct CL_Kernel {
  cl_kernel kernel;
  std::map<unsigned int, MemObject*> arg_cl_mem;
  bool skip;
};

#endif  // NEWSCL_CN_WARPPED_CL_TYPE_H_
