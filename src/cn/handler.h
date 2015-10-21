// Copyright [2013] (JoenChen)
#ifndef NEWSCL_CN_HANDLER_H_
#define NEWSCL_CN_HANDLER_H_
#include <vector>

#include <CL/cl.h>

#include "./command.h"
#include "./wrapped_CL_type.h"
//#include "../../../BasicGpuDemo/BasicGpuDemo.h"

void HandlerRegister(int sock,
                     char *recv_buf);

void HandlerBullet3ReadResultBack(int sock,
                                  char *recv_buf,
                                  BasicGpuDemo &demo,
                                  std::vector<std::shared_ptr<CommandQueue> > &comm_queues);

void HandlerCreateProgramWithSource(int sock,
                                    char *recv_buf,
                                    std::vector<cl_program> &programs);

void HandlerBuildProgram(int sock,
                         char *recv_buf,
                         std::vector<cl_program> &programs);

void HandlerCreateKernel(int sock,
                         char *recv_buf,
                         std::vector<cl_program> &programs,
                         std::vector<CL_Kernel> &kernels);

void HandlerEnqueueTask(int sock,
                        char *recv_buf,
                        std::vector<CL_Kernel> &kernels,
                        CommandQueue& comm_queue);

void HandlerEnqueueNDRangeKernel(int sock,
                        char *recv_buf,
                        std::vector<CL_Kernel> &kernels,
                        std::vector<MemObject> &mems,
                        CommandQueue& comm_queue);

void HandlerCreateBuffer(int sock,
                         char *recv_buf,
                         std::vector<MemObject> &mems);

void HandlerEnqueueWriteBuffer(int sock,
                               char *recv_buf,
                               std::vector<MemObject> &mems,
                               const std::vector<std::shared_ptr<CommandQueue> > &comm_queue,
                               int non_blocking_sock);

void HandlerEnqueueReadBuffer(int sock,
                              char *recv_buf,
                              std::vector<MemObject> &mems,
                              CommandQueue& comm_queue,
                              int non_blocking_sock);

void HandlerEnqueueCopyBuffer(int sock,
                              char *recv_buf,
                              std::vector<MemObject> &mems,
                              CommandQueue& comm_queue);

void HandlerSetKernelArg(int sock,
                         char *recv_buf,
                         std::vector<CL_Kernel> &kernels,
                         std::vector<MemObject> &mems);

void HandlerFinish(int sock,
                   char *recv_buf,
                   std::vector<std::shared_ptr<CommandQueue> >& comm_queues);

void HandlerReleaseMemObject(int sock,
                             char *recv_buf,
                             std::vector<MemObject> &mems,
                             CommandQueue& comm_queue);
#endif  // NEWSCL_CN_HANDLER_H_
