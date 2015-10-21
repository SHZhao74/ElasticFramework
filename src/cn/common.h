#ifndef NEWSCL_CN_COMMON_H_
#define NEWSCL_CN_COMMON_H_

#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<CL/opencl.h>
#include<assert.h>

double GetCPUTime();

double GetCLExecTime(cl_event event);
#define GetErrorMsg(a) {cl_int ED_OpenCLErrorCode_ret = (a); if((ED_OpenCLErrorCode_ret) != CL_SUCCESS){ fprintf(stderr, "file: %s line: %d ErrorMsg: (%d)%s\n", __FILE__, __LINE__, (ED_OpenCLErrorCode_ret), GetOpenCLErrorCodeStr((ED_OpenCLErrorCode_ret))); assert(false);}}
#define _ED(a) GetErrorMsg((a))
int GetBuildLog(int errorCode, cl_program program, cl_device_id device_id);
const char* GetOpenCLErrorCodeStr(int errorCode);
#endif
