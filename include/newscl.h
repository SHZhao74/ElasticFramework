// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_NEWSCL_H_
#define NEWSCL_NEWSCL_H_
#include "CL/opencl.h"

#include <string>
//#include "cl_api.h"
#include "IO.h"
namespace newscl{
	namespace dispatcher_msg_type {
		static const unsigned int kGetBindingCN = 0;
		static const unsigned int kLoadInfo = 1;
		static const unsigned int kCNReg = 2;
		static const unsigned int kGetBestCN =3;
		static const unsigned int kUpdateScore = 4;
	};
  namespace cn_msg_type{
    const unsigned int kReg = 0;
		const unsigned int kCreateProgramWithSource = 1;
		const unsigned int kBuildProgram= 2;
		const unsigned int kCreateKernel= 3;
		const unsigned int kEnqueueTask = 4;
		const unsigned int kCreateBuffer = 5;
		const unsigned int kEnqueueReadBuffer = 6;
		const unsigned int kEnqueueWriteBuffer = 7;
		const unsigned int kSetKernelArg = 8;
		const unsigned int kFinish = 9;
		const unsigned int kEnqueueNDRangeKernel = 10;
		const unsigned int kEnqueueCopyBuffer = 11;
		const unsigned int kReleaseMemObject = 12;
    const unsigned int kBullet3ReadResultBack = 13;
    const unsigned int kBullet3NPWriteToGPU = 14;
    const unsigned int kBullet3RBPWriteToGPU = 15;
    const unsigned int kBullet3BPWriteToGPU = 16;
  }
	namespace dev_type{
		static const unsigned int GPU = 0;
		static const unsigned int CPU = 1;
  };
	const std::string kPlatformName = "NEWSCL";
	const unsigned int kMsgMaxSize = 64;
};
#endif  // NEWSCL_NEWSCL_H_
