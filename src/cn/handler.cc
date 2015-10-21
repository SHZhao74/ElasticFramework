// Copyright [2013] (JoenChen)
#include "./handler.h"

#include <string.h>
#include <stdio.h>

#include <vector>
#include <array>
#include <memory>
#include <algorithm>

#include "./common.h"
#include "./cn.h"
#include "./cn_info.h"
#include "IO.h"
#include "newscl.h"
//#include "../../../BasicGpuDemo/BasicGpuDemo.h"
//#include "../../../BasicGpuDemo/b3GpuDynamicsWorld.h"
//#include "../../../../src/Bullet3OpenCL/ParallelPrimitives/b3OpenCLArray.h"
//#include "../../../../src/Bullet3Collision/NarrowPhaseCollision/b3RigidBodyCL.h"

static void Paste(char *buf, size_t *buf_offset, void *ptr, size_t size) {
  memcpy(buf + *buf_offset, ptr, size);
  *buf_offset += size;
}

void HandlerRegister(int sock,
                     char *recv_buf) {
  std::array<char, newscl::kMsgMaxSize> send_buf;
  auto &cn_info = newscl::CNInfo::Instance();

  double app_weight = *(reinterpret_cast<double*>(recv_buf));

  cn_info->set_weight(cn_info->weight() + app_weight);
  snprintf(send_buf.data(), newscl::kMsgMaxSize, "Reg succeed");
  newscl::SockSend(sock, send_buf.data(), send_buf.size());
}

void HandlerFinish(int sock,
                   char *recv_buf,
                   std::vector<std::shared_ptr<CommandQueue> > &comm_queues) {
  int queue_id = *(reinterpret_cast<unsigned int*>(recv_buf));
  CommandQueue &queue = *comm_queues[queue_id];
  CommandFinish *comm_finish = new CommandFinish();
  Command *comm = dynamic_cast<Command*>(comm_finish);
  //fprintf(stderr, "comm_finish:%f\n", comm->est_exec_time());
  queue.Push(comm);

#ifdef DEBUG
  printf("pool sem: %d\n", CommandPool::Instance()->SemValue());
  printf("start waiting\n");
#endif // DEBUG
  fprintf(stderr, "starting waiting finishing\n");
  comm_finish->Wait();
  fprintf(stderr, "ending waiting finishing\n");
#ifdef DEBUG
  printf("end waiting\n");
#endif // DEBUG
  /*
  queue.WaitForFinishing();
  */

  std::array<char, newscl::kMsgMaxSize> send_buf;
  size_t buf_offset = 0;
  cl_int errcode_ret = CL_SUCCESS;
  Paste(send_buf.data(),
        &buf_offset,
        reinterpret_cast<void*>(&errcode_ret),
        sizeof(errcode_ret));

  fprintf(stderr, "start ack finishing\n");
  newscl::SockSend(sock, send_buf.data(), newscl::kMsgMaxSize);
  fprintf(stderr, "end ack finishing\n");
}
void HandlerCreateProgramWithSource(int sock,
                                    char *recv_buf,
                                    std::vector<cl_program> &programs) {
  int count = *(reinterpret_cast<int*>(recv_buf + sizeof(int)));
  size_t* lengths;
  int error_code;
  lengths = new size_t[count];
  for (int i = 0; i < count; ++i) {
    newscl::SockRecv(sock, reinterpret_cast<char*>(&lengths[i]), sizeof(size_t));
  }

  char **strs;
  strs = new char* [count];
  for (int i = 0; i < count; ++i) {
    strs[i] = new char[lengths[i] + 1];
    strs[i][lengths[i]] = '\0';
    newscl::SockRecv(sock, strs[i], lengths[i]);
    //printf("code %d: %s\n", i, strs[i]);
  }
  cl_program program = clCreateProgramWithSource(contexts[0],
                                                            count,
                                                            (const char**)strs,
                                                            lengths,
                                                            &error_code);
  GetErrorMsg(error_code);
  unsigned int program_id = programs.size();
  programs.push_back(program);

  char send_buf[newscl::kMsgMaxSize];
  size_t buf_offset = 0;
  bzero(send_buf, newscl::kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&program_id),
        sizeof(program_id));
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&error_code),
        sizeof(error_code));
  
  newscl::SockSend(sock,send_buf, newscl::kMsgMaxSize);

  for (int i = 0; i < count ; ++i) {
    delete[] strs[i];
  }
  delete[] strs;
}

void HandlerBuildProgram(int sock,
                         char *recv_buf,
                         std::vector<cl_program> &programs) {
  unsigned int program_id = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                              sizeof(int)));
  cl_int err_code;
  char send_buf[newscl::kMsgMaxSize];
  size_t buf_offset = 0;
#ifdef DEBUG
  printf("program_id: %d\n", program_id);
  printf("program_size: %ld\n", programs.size());
  printf("devs size: %ld\n", devs.size());
#endif // DEBUG

  _ED(err_code = clBuildProgram(programs[program_id],
                                devs.size(),
                                &devs.front(),
                                NULL,
                                NULL,
                                NULL));
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&err_code),
        sizeof(err_code));
  newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);
}

void HandlerCreateKernel(int sock,
                         char *recv_buf,
                         std::vector<cl_program> &programs,
                         std::vector<CL_Kernel> &kernels) {
  unsigned int program_id = *(reinterpret_cast<unsigned int*>(recv_buf));
  unsigned int kernel_name_length = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                                      sizeof(program_id)));
  char *kernel_name = new char[kernel_name_length + 1];
  cl_int err_code;
  fprintf(stderr, "__LINE__:%d\n", __LINE__);
  kernel_name[kernel_name_length] = '\0';
  if (kernel_name == NULL) {
    assert(false);  // allocate kernel_name failed
    return;
  }
  fprintf(stderr, "__LINE__:%d\n", __LINE__);
  newscl::SockRecv(sock, kernel_name, kernel_name_length);
  fprintf(stderr, "__LINE__:%d\n", __LINE__);
#ifdef DEBUG
  printf("kernel name length: %d\n", total_recv_size);
  printf("kernel name: %s\n", kernel_name);
#endif // DEBUG
  fprintf(stderr, "__LINE__:%d\n", __LINE__);
  kernels.resize(kernels.size() + 1);
  kernels.back().kernel = clCreateKernel(programs[program_id],
                                         kernel_name,
                                         &err_code);
  kernels.back().skip = false;
  fprintf(stderr, "__LINE__:%d\n", __LINE__);
  GetErrorMsg(err_code);
  fprintf(stderr, "__LINE__:%d\n", __LINE__);

  // echo
  char send_buf[newscl::kMsgMaxSize] = {};
  size_t buf_offset = 0;
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&err_code),
        sizeof(err_code));
  snprintf(send_buf + sizeof(err_code),
           newscl::kMsgMaxSize - sizeof(err_code),
           "kernel id:%ld",
           kernels.size() -1);
  newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);
}

void HandlerEnqueueTask(int sock,
                        char *recv_buf,
                        std::vector<CL_Kernel> &kernels,
                        CommandQueue &comm_queue) {
  unsigned int kernel_id = *(reinterpret_cast<unsigned int*> (recv_buf));
  //int errcode_ret = CL_SUCCESS;
  CommandKernel *comm_kernel = new CommandKernel(&kernels[kernel_id], false, kernel_id);

  Command *comm = dynamic_cast<Command*>(comm_kernel);
  comm_queue.Push(comm);

  // echo
  //char send_buf[newscl::kMsgMaxSize] = {};
  //size_t buf_offset = 0;
  //bzero(send_buf, newscl::kMsgMaxSize);
  //Paste(send_buf,
  //      &buf_offset,
  //      reinterpret_cast<void*>(&errcode_ret),
  //      sizeof(errcode_ret));
  //newscl::SockSend(sock,send_buf, newscl::kMsgMaxSize);
}
void HandlerEnqueueNDRangeKernel(int sock,
                        char *recv_buf,
                        std::vector<CL_Kernel> &kernels,
                         std::vector<MemObject> &mems,
                        CommandQueue &comm_queue) {
  unsigned int kernel_id = *(reinterpret_cast<unsigned int*> (recv_buf));
  //char kernel_name[100];
  //_ED(clGetKernelInfo(kernels[kernel_id].kernel, CL_KERNEL_FUNCTION_NAME, 100, kernel_name, NULL));
  //fprintf(stderr, "Kernel Name: %s\n", kernel_name);
  //if(strcmp(kernel_name, "flipFloatKernel") == 0) {
  //  int a=1+2;
  //  a++;
  //}
  unsigned char work_dim = *(reinterpret_cast<unsigned char*>(recv_buf +
                                                        sizeof(kernel_id)));
  size_t *global_work_size = reinterpret_cast<size_t*>(recv_buf +
                                                       sizeof(kernel_id) + 
                                                       sizeof(work_dim));

  size_t *local_work_size = reinterpret_cast<size_t*>(recv_buf +
                                                      sizeof(kernel_id) + 
                                                      sizeof(work_dim) +
                                                      sizeof(size_t) * 3);
  unsigned char num_of_changed_mem_id =
    *reinterpret_cast<unsigned char*>(recv_buf +
                                      sizeof(kernel_id) +
                                      sizeof(work_dim) +
                                      sizeof(*global_work_size) * 3 +
                                      sizeof(*local_work_size) * 3);
  unsigned char num_of_const_data =
    *reinterpret_cast<unsigned char*>(recv_buf +
                                      sizeof(kernel_id) +
                                      sizeof(work_dim) +
                                      sizeof(*global_work_size) * 3 +
                                      sizeof(*local_work_size) * 3 +
                                      sizeof(num_of_changed_mem_id));
  //fprintf(stderr, "num_of_const_data:%d\n", num_of_const_data);
  unsigned int const_data_size =
    *reinterpret_cast<unsigned char*>(recv_buf +
                                      sizeof(kernel_id) +
                                      sizeof(work_dim) +
                                      sizeof(*global_work_size) * 3 +
                                      sizeof(*local_work_size) * 3 +
                                      sizeof(num_of_changed_mem_id) +
                                      sizeof(num_of_const_data));
  CommandKernel *comm_kernel = new CommandKernel(&kernels[kernel_id],
                                                 true,
                                                 kernel_id,
                                                 work_dim,
                                                 nullptr,
                                                 global_work_size,
                                                 local_work_size
                                                 );

  Command *comm = dynamic_cast<Command*>(comm_kernel);

  //fprintf(stderr, "num_of_change_mem_id:%d\n", num_of_changed_mem_id);
  if (num_of_changed_mem_id != 0) {
    std::array<char, newscl::kMsgMaxSize> arg_recv_buf;
    const int max_arg_in_msg = newscl::kMsgMaxSize /
      (sizeof(unsigned char) + sizeof(unsigned int));
    const int send_times = (num_of_changed_mem_id + max_arg_in_msg - 1) /
                           max_arg_in_msg;
    unsigned char arg_index = 0;
    unsigned int arg_mem_id = 0;
    for (int i = 0; i < send_times; ++i) { 
      const int end_j =
        (i == send_times - 1?num_of_changed_mem_id : (i + 1) * max_arg_in_msg);
      newscl::SockRecv(sock, arg_recv_buf.data(), arg_recv_buf.size());
      for (int j = i * max_arg_in_msg; j < end_j; ++j) {
        arg_index = *reinterpret_cast<unsigned char*>(arg_recv_buf.data() +
                                                      (sizeof(arg_index) +
                                                       sizeof(arg_mem_id)) *
                                                      (j % max_arg_in_msg));
        arg_mem_id = *reinterpret_cast<unsigned int*>(arg_recv_buf.data() +
                                                      (sizeof(arg_index) +
                                                       sizeof(arg_mem_id)) *
                                                      (j % max_arg_in_msg) +
                                                      sizeof(arg_index));
#ifdef DEBUG
        printf("arg_index: %d, arg_mem_id: %d\n", arg_index, arg_mem_id);
#endif // DEBUG
        //fprintf(stderr, "arg_index: %d, arg_mem_id: %d\n", arg_index, arg_mem_id);
        if (arg_mem_id != 2147483247) {
          comm_kernel->AddArg(arg_index, mems[arg_mem_id].mem);
          //_ED(clSetKernelArg(kernels[kernel_id].kernel,
          //                 arg_index,
          //                 sizeof(cl_mem),
          //                 &(mems[arg_mem_id].mem)));
        } else {
          assert(false);
          _ED(clSetKernelArg(kernels[kernel_id].kernel,
                           arg_index,
                           256 * 4 * sizeof(float),
                           NULL));
        }
      }
    }
    //fprintf(stderr, "memSendTime:%d", send_times);
  }
  //std::vector<char> const_data(const_data_size);
  if (const_data_size > 0) {
    comm_kernel->const_data.resize(const_data_size);
    newscl::SockRecv(sock,
                     comm_kernel->const_data.data(),
                     comm_kernel->const_data.size());
  }
  if (num_of_const_data != 0) {
    unsigned int recv_const_data = 0;
    std::array<char, newscl::kMsgMaxSize> const_recv_buf;
    const int kMaxConstInMsg = newscl::kMsgMaxSize /
      (sizeof(unsigned char) + sizeof(size_t));
    const int kConstRecvTimes = (num_of_const_data + kMaxConstInMsg - 1) / kMaxConstInMsg;
    //fprintf(stderr, "num_of_const_data:%d kConstInMsg:%d\n", num_of_const_data, kMaxConstInMsg);
    unsigned char arg_index = 0;
    size_t arg_size = 0;
    for (int i = 0; i < kConstRecvTimes; ++i) { 
      newscl::SockRecv(sock, const_recv_buf.data(), const_recv_buf.size());
      const int kEndJ =
        (i == kConstRecvTimes - 1?num_of_const_data : (i + 1) * kMaxConstInMsg);
      for (int j = i * kMaxConstInMsg; j < kEndJ; ++j) {
        arg_index = *reinterpret_cast<unsigned char*>(const_recv_buf.data() +
                                                      (sizeof(arg_index) +
                                                       sizeof(arg_size)) *
                                                      (j % kMaxConstInMsg));
        arg_size = *reinterpret_cast<size_t*>(const_recv_buf.data() +
                                              (sizeof(arg_index) +
                                               sizeof(arg_size)) *
                                              (j % kMaxConstInMsg) +
                                              sizeof(arg_index));
        recv_const_data += arg_size;
        //fprintf(stderr, "arg_index: %d, arg_size: %ld\n", arg_index, arg_size);
        comm_kernel->AddConst(arg_index, arg_size);
#ifdef DEBUG
        printf("arg_index: %d, arg_mem_id: %d\n", arg_index, arg_mem_id);
#endif // DEBUG
        //fprintf(std
      }
    }
    assert(recv_const_data == const_data_size);
    //fprintf(stderr, "kConstRecvTime:%d\n", kConstRecvTimes);
  }
  comm_queue.Push(comm);

  //int errcode_ret = CL_SUCCESS;
  //char send_buf[newscl::kMsgMaxSize] = {};
  //size_t buf_offset = 0;
  //bzero(send_buf, newscl::kMsgMaxSize);
  //Paste(send_buf,
  //      &buf_offset,
  //      reinterpret_cast<void*>(&errcode_ret),
  //      sizeof(errcode_ret));
  //newscl::SockSend(sock,send_buf, newscl::kMsgMaxSize);
}
void HandlerCreateBuffer(int sock,
                         char *recv_buf,
                         std::vector<MemObject> &mems) {
  cl_mem_flags flags = *(reinterpret_cast<cl_mem_flags*>(recv_buf));
  size_t size = *(reinterpret_cast<size_t*>(recv_buf + sizeof(flags)));
  char *data = new char[size];
  cl_int errcode_ret = CL_SUCCESS;
  MemObject mem;
  if ((flags & CL_MEM_COPY_HOST_PTR) == CL_MEM_COPY_HOST_PTR) {
    newscl::SockRecv(sock, data, size);
    mem.mem = clCreateBuffer(contexts.front(),
                                      flags,
                                      size,
                                      data,
                                      &errcode_ret);
    GetErrorMsg(errcode_ret);
  } else {
    //mems.resize(mems.size() + 1);
    mem.mem = clCreateBuffer(contexts.front(),
                                      flags,
                                      size,
                                      NULL,
                                      &errcode_ret);
    GetErrorMsg(errcode_ret);
  }
  mem.size = size;
  //fprintf(stderr, "memory object size size:%ld\n",size);
#ifdef DEBUG
  fprintf(stderr, "size:%ld\n",size);
#endif // DEBUG
  assert(size != 0);
  mem.newest_on_dev = nullptr;
  //fprintf(stderr, "mem object address:%p\n", mem.mem);
  mems.push_back(mem);
  //fprintf(stderr, "mem object address:%p\n", mems.back().mem);
  //fprintf(stderr, "mem created");
  //unsigned int mem_id = mems.size() - 1;

  //char send_buf[newscl::kMsgMaxSize];
  //size_t buf_offset = 0;
  //bzero(send_buf, newscl::kMsgMaxSize);
  //Paste(send_buf,
  //      &buf_offset,
  //      reinterpret_cast<void*>(&errcode_ret),
  //      sizeof(errcode_ret));
  //Paste(send_buf,
  //      &buf_offset,
  //      reinterpret_cast<void*>(&mem_id),
  //      sizeof(mem_id));

  //fprintf(stderr, "ack start\n");
  //newscl::SockSend(sock,send_buf, newscl::kMsgMaxSize);
  //fprintf(stderr, "ack end\n");

  delete[] data;
#ifdef DEBUG
  printf("mem_id:%d\n", mem_id);
#endif // DEBUG
}

void HandlerEnqueueCopyBuffer(int sock,
                              char *recv_buf,
                              std::vector<MemObject> &mems,
                              CommandQueue& comm_queue) {
  unsigned int src_mem_id = *(reinterpret_cast<unsigned int*>(recv_buf));
  unsigned int dst_mem_id = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                              sizeof(src_mem_id)));
  size_t src_offset = *(reinterpret_cast<size_t*>(recv_buf +
                                                  sizeof(src_mem_id) +
                                                  sizeof(dst_mem_id)));
  size_t dst_offset = *(reinterpret_cast<size_t*>(recv_buf +
                                                  sizeof(src_mem_id) +
                                                  sizeof(dst_mem_id) +
                                                  sizeof(src_offset)));
  size_t cb = *(reinterpret_cast<size_t*>(recv_buf +
                                          sizeof(src_mem_id) +
                                          sizeof(dst_mem_id) +
                                          sizeof(src_offset) +
                                          sizeof(dst_offset)));
  //std::vector<char> data(mems[src_mem_id].size);
  Command *comm = new CommandCopyBuffer(mems[src_mem_id].mem,
                                        mems[dst_mem_id].mem,
                                        src_offset,
                                        dst_offset,
                                        cb);
  comm_queue.Push(comm);


/*
  _ED(clEnqueueCopyBuffer(queues[0],
                          mems[src_mem_id].mem,
                          mems[dst_mem_id].mem,
                          src_offset,
                          dst_offset,
                          cb,
                          0,
                          NULL,
                          NULL));
                          */
  
  /*
  if (mems[src_mem_id].newest_on_dev == nullptr) {
    _ED(clEnqueueReadBuffer(queues[0], mems[src_mem_id].mem, true,
                            src_offset, cb, data.data(), 0, NULL, NULL));
  } else {
    _ED(clEnqueueReadBuffer(mems[src_mem_id].newest_on_dev, mems[src_mem_id].mem,
                            true, src_offset, cb, data.data(), 0, NULL, NULL));
  }

  if (mems[dst_mem_id].newest_on_dev == nullptr) {
    for (const auto &x : queues) {
      _ED(clEnqueueWriteBuffer(x, mems[dst_mem_id].mem, true,
                               dst_offset, cb, data.data(), 0, NULL, NULL));
    }
  } else {
    _ED(clEnqueueWriteBuffer(mems[dst_mem_id].newest_on_dev, mems[dst_mem_id].mem,
                             true, dst_offset, cb, data.data(), 0, NULL, NULL));
  }
  */
  //char send_buf[newscl::kMsgMaxSize];
  //bzero(send_buf, newscl::kMsgMaxSize);
  //newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);

}

void HandlerEnqueueWriteBuffer(int sock,
                               char *recv_buf,
                               std::vector<MemObject> &mems,
                               const std::vector<std::shared_ptr<CommandQueue> > &queues,
                               int non_blocking_sock) {
  size_t offset = *(reinterpret_cast<size_t*>(recv_buf));
  size_t cb = *(reinterpret_cast<size_t*>(recv_buf + sizeof(offset)));
  unsigned int mem_id = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                          sizeof(offset) +
                                                          sizeof(cb)));
  cl_bool blocking_write = *(reinterpret_cast<cl_bool *>(recv_buf +
                                                         sizeof(offset) +
                                                         sizeof(cb) +
                                                         sizeof(mem_id)));
  //fprintf(stderr, "when Write Buffering, mem addr:%p\n", mems[mem_id].mem);
  //fprintf(stderr, "when Write Buffering, mem_id:%d\n", mem_id);
  //fprintf(stderr, "when Write Buffering, mem_size:%ld\n", mems.size());
  //fprintf(stderr, "when Write Buffering, mem array size:%ld\n", mems.size());
  //fprintf(stderr, "when Write Buffering, mem size:%d\n", mems[mem_id].size);
#ifdef DEBUG
#endif // DEBUG
  std::vector<char> data(cb);
  size_t queue_id = 0;
  auto& comm_queue = queues[queue_id];
  assert(queues.size() > 0);
  newscl::SockRecv(sock, data.data(), cb);
  CommandWrite *comm_write = new CommandWrite(
                                             &mems[mem_id],
                                             blocking_write,
                                             offset,
                                             cb,
                                             static_cast<cl_event>(nullptr),
                                             data);
  comm_write->mem_object = mems[mem_id].mem;
 Command *comm = dynamic_cast<Command*>(comm_write);
  //fprintf(stderr, "when Write Buffering, mem address:%p\n", comm_write->mem_->mem);
  if (blocking_write) {
    comm_queue->Push(comm);
    comm_write->Blocking();
    cl_int err_code = comm_write->err_code();
    
    char send_buf[newscl::kMsgMaxSize];
    size_t buf_offset = 0;
    bzero(send_buf, newscl::kMsgMaxSize);
    Paste(send_buf,
          &buf_offset,
          reinterpret_cast<void*>(&err_code),
          sizeof(err_code));
    Paste(send_buf,
          &buf_offset,
          reinterpret_cast<void*>(&mem_id),
          sizeof(mem_id));

    newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);
  } else {
    comm_write->set_non_blocking_sock(non_blocking_sock);
    //comm->Exec(::queues.front());
    //fprintf(stderr,"press any key");
    //fgetc(stdin);
    comm_queue->Push(comm);
  }
}

void HandlerEnqueueReadBuffer(int sock,
                              char *recv_buf,
                              std::vector<MemObject> &mems,
                              CommandQueue& comm_queue,
                              int non_blocking_sock) {
  size_t offset = *(reinterpret_cast<size_t*>(recv_buf));
  size_t cb = *(reinterpret_cast<size_t*>(recv_buf + sizeof(offset)));
  unsigned int mem_id = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                          sizeof(offset) +
                                                          sizeof(cb)));
  cl_bool blocking_read = *(reinterpret_cast<cl_bool *>(recv_buf +
                                                        sizeof(offset) +
                                                        sizeof(cb) +
                                                        sizeof(mem_id)));

  CommandRead *comm_read = new CommandRead(
                                           &mems[mem_id],
                                           blocking_read,
                                           offset,
                                           cb,
                                           static_cast<cl_event>(nullptr));
  //fprintf(stderr, "mem_id:%d offset:%ld cb:%ld\n", mem_id, offset, cb);

  Command* comm = dynamic_cast<Command*>(comm_read);
  if (blocking_read) {
    comm_queue.Push(comm);
    comm_read->Blocking();
    
    char send_buf[newscl::kMsgMaxSize];
    size_t buf_offset = 0;
    cl_int err_code = comm_read->err_code();
    bzero(send_buf, newscl::kMsgMaxSize);
    Paste(send_buf,
          &buf_offset,
          reinterpret_cast<void*>(&err_code),
          sizeof(err_code));
    newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);

    newscl::SockSend(sock, comm_read->getData().data(), cb);
  } else {
    comm_read->set_non_blocking_sock(non_blocking_sock);
    comm_queue.Push(comm);
  }
}
void HandlerSetKernelArg(int sock,
                         char *recv_buf,
                         std::vector<CL_Kernel> &kernels,
                         std::vector<MemObject> &mems) {
  assert(false);
  cl_int errcode_ret;
  unsigned int kernel_id = *(reinterpret_cast<unsigned int*>(recv_buf));
  cl_uint arg_index = *(reinterpret_cast<cl_uint*>(recv_buf + sizeof(kernel_id)));
  bool is_pointer = *(reinterpret_cast<bool*>(recv_buf +
                                              sizeof(kernel_id) +
                                              sizeof(arg_index)));
  if (is_pointer) {
    assert(false);
    unsigned int mem_id = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                            sizeof(kernel_id) +
                                                            sizeof(arg_index) + 
                                                            sizeof(is_pointer)));
#ifdef DEBUG
    fprintf(stderr, "is_pointer, mem_id: %d", mem_id);
#endif // DEBUG
    _ED(errcode_ret = clSetKernelArg(kernels[kernel_id].kernel,
                                     arg_index,
                                     sizeof(cl_mem),
                                     &(mems[mem_id].mem)));
    kernels[kernel_id].arg_cl_mem[arg_index] = &mems[mem_id];
  } else {
    unsigned int arg_size = *(reinterpret_cast<unsigned int*>(recv_buf +
                                                              sizeof(kernel_id) +
                                                              sizeof(arg_index) +
                                                              sizeof(is_pointer)));
    std::vector<char> data(arg_size);
    newscl::SockRecv(sock, data.data(), data.size());
    _ED(errcode_ret = clSetKernelArg(kernels[kernel_id].kernel,
                                     arg_index,
                                     arg_size,
                                     data.data()));
  }
  char send_buf[newscl::kMsgMaxSize];
  size_t buf_offset = 0;
  bzero(send_buf, newscl::kMsgMaxSize);
  Paste(send_buf, &buf_offset, reinterpret_cast<void*>(&errcode_ret), sizeof(errcode_ret));

  newscl::SockSend(sock, send_buf, newscl::kMsgMaxSize);
}
void HandlerReleaseMemObject(int sock,
                             char *recv_buf,
                             std::vector<MemObject> &mems,
                             CommandQueue& comm_queue) {
  unsigned int mem_id = *(reinterpret_cast<unsigned int*>(recv_buf));
  //for (auto &queue: queues) {
  //  clFinish(queue);
  //}
  //_ED(clReleaseMemObject(mems[mem_id].mem));
  Command *comm = new CommandReleaseMemObject(mems[mem_id].mem);
  comm_queue.Push(comm);
}
