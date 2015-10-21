// Copyright [2013] (JoenChen: joen@joen.cc)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h>

#include <array>
#include <cassert>
#include <memory>

#include <boost/property_tree/ptree.hpp>                                         
#include <boost/property_tree/json_parser.hpp>     

#include "CL/cl.h"
#include "CL/opencl.h"
#include "CL/cl_ext.h"
#include "./cl_dispatch.h"
#include "./cl_api.h"
#include "./runtime.h"
#include "./parser.h"
#include "newscl.h"
#include "config.h"
#include <vector>
#include <algorithm>
#include <iterator>
#define DEBUG
double& newscl::GetWeight(double const &t_weight){
  static double weight = t_weight;
  static double is_init = false;
  return weight;
}
newscl::SocketIOClient* GetNonBlockingConnection2(const std::string kCNHost = "") {
  static newscl::SocketIOClient conn(kCNHost,
                                     newscl::kCNNonBlockingPort);
  static bool is_init = false;
  if (!is_init) {
    conn.Connect();
    is_init = true;
    int flag = 1;
    int result = setsockopt(conn.sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
    assert(result>=0);
  }
  return &conn;
}
newscl::SocketIOClient* GetNonBlockingConnection(int no = 1, const std::string kCNHost = "") {
  static std::vector<newscl::SocketIOClient> connVector;
  if(kCNHost.size() > 0) {
    newscl::SocketIOClient *conn = new newscl::SocketIOClient(kCNHost, newscl::kCNNonBlockingPort);
    conn->Connect();
    int flag = 1;
    int result = setsockopt(conn->sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
      //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}


static void Paste(char *buf, size_t *buf_offset, void *ptr, size_t size) {
  memcpy(buf + *buf_offset, ptr, size);
  *buf_offset += size;
}


cl_int newscl::clReleaseMemObject(cl_mem memobj){
  //return CL_SUCCESS;
  
  int msg_type_num = newscl::cn_msg_type::kReleaseMemObject;
  std::array<char, kMsgMaxSize> send_buf;
  size_t buf_offset =0;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&memobj->id),
        sizeof(memobj->id));
  GetConnection()->Send(send_buf.data(), newscl::kMsgMaxSize);

  std::array<char, kMsgMaxSize> recv_buf;
  delete memobj;
  //GetConnection()->Recv(recv_buf.data(), newscl::kMsgMaxSize);
  //return *reinterpret_cast<cl_int*>(recv_buf.data());
  return CL_SUCCESS;
  
}

cl_int newscl::clReleaseProgram(cl_program) {
  return CL_SUCCESS;
}
cl_int newscl::clEnqueueCopyBuffer(cl_command_queue command_queue,
                           cl_mem src_buffer,
                           cl_mem dst_buffer,
                           size_t src_offset,
                           size_t dst_offset,
                           size_t cb,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event) {
  if (command_queue != GetCommandQueue()) {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (src_buffer == dst_buffer &&
      ((src_offset <= dst_offset && (src_offset + cb) > dst_offset) ||
       (dst_offset <  src_offset && (dst_offset + cb) > src_offset))) {
    return CL_MEM_COPY_OVERLAP;
  }
  if ((src_offset + cb > src_buffer->size) ||
      (dst_offset + cb > dst_buffer->size)) {
    return CL_INVALID_VALUE;
  }
#ifdef DEBUG_
  fprintf(stderr, "Copy src_mem_id:%d dst_mem_id:%d cb:%ld\n", src_buffer->id
                                                            , dst_buffer->id
                                                            , cb);
#endif //DEBUG_
  int msg_type_num = newscl::cn_msg_type::kEnqueueCopyBuffer;
  std::array<char, kMsgMaxSize> send_buf;
  std::array<char, kMsgMaxSize> recv_buf;
  size_t buf_offset = 0;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&src_buffer->id),
        sizeof(src_buffer->id));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&dst_buffer->id),
        sizeof(dst_buffer->id));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&src_offset),
        sizeof(src_offset));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&dst_offset),
        sizeof(dst_offset));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&cb),
        sizeof(cb));
  GetConnection()->Send(send_buf.data(), kMsgMaxSize);

  //GetConnection()->Recv(recv_buf.data(), kMsgMaxSize);

  //return *(reinterpret_cast<cl_int*>(recv_buf.data()));
  return CL_SUCCESS;
}

cl_int newscl::clFinish(cl_command_queue) {
  int msg_type_num = newscl::cn_msg_type::kFinish;
  std::array<char, kMsgMaxSize> send_buf;
  std::array<char, kMsgMaxSize> recv_buf;
  size_t buf_offset = 0;
  unsigned int queue_id = 0;
#ifdef DEBUG_
  fprintf(stderr, "FINISH\n");
#endif //DEBUG_
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&queue_id),
        sizeof(queue_id));
  GetConnection()->Send(send_buf.data(), kMsgMaxSize);
  GetFinishConnection()->Recv(recv_buf.data(), kMsgMaxSize);

  return *(reinterpret_cast<cl_int*>(recv_buf.data()));
}


_cl_context* newscl::clCreateContextFromType(
    const cl_context_properties*,
    cl_device_type,
    void(CL_CALLBACK *)(
      const char*,
      const void*,
      size_t, void*),
    void*,
    cl_int* errcode_ret) {
  if (errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;
  return GetContext();
}



cl_int newscl::clGetContextInfo(
    cl_context context,
    cl_context_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t* param_value_size_ret) {
  cl_device_id *ids;
  if (context != GetContext()) {
    return CL_INVALID_CONTEXT;
  }
  switch (param_name) {
    case CL_CONTEXT_DEVICES: {
      if (param_value != NULL) {
        if (param_value_size < sizeof(cl_device_id)) {
          return CL_INVALID_VALUE;
        }
        ids = static_cast<cl_device_id*>(param_value);
        *ids = GetDevice();
      }
      if (param_value_size_ret != NULL) {
        *param_value_size_ret = sizeof(cl_device_id);
      }
      break;
    }
    default: {
      return CL_INVALID_VALUE;
    }
  }

  return CL_SUCCESS;
}


_cl_icd_dispatch* newscl::GetDispatch() {
  static _cl_icd_dispatch dispatch;
  static bool is_init = false;
  if (!is_init) {
    dispatch.clGetDeviceIDs = newscl::clGetDeviceIDs;
    dispatch.clGetDeviceInfo = newscl::clGetDeviceInfo;
    dispatch.clGetPlatformInfo = newscl::clGetPlatformInfo;
    dispatch.clCreateProgramWithSource = newscl::clCreateProgramWithSource;
    dispatch.clBuildProgram = newscl::clBuildProgram;
    dispatch.clCreateContextFromType = newscl::clCreateContextFromType;
    dispatch.clGetContextInfo = newscl::clGetContextInfo;
    dispatch.clGetPlatformIDs = newscl::clGetPlatformIDs;
    dispatch.clCreateContext = newscl::clCreateContext;
    dispatch.clCreateKernel = newscl::clCreateKernel;
    dispatch.clCreateCommandQueue = newscl::clCreateCommandQueue;
    dispatch.clEnqueueTask = newscl::clEnqueueTask;
    dispatch.clEnqueueNDRangeKernel = newscl::clEnqueueNDRangeKernel;
    dispatch.clCreateBuffer = newscl::clCreateBuffer;
    dispatch.clEnqueueReadBuffer = newscl::clEnqueueReadBuffer;
    dispatch.clEnqueueWriteBuffer = newscl::clEnqueueWriteBuffer;
    dispatch.clSetKernelArg = newscl::clSetKernelArg;
    dispatch.clFlush = newscl::clFinish;
    dispatch.clFinish = newscl::clFinish;
    dispatch.clGetMemObjectInfo = newscl::clGetMemObjectInfo;
    dispatch.clEnqueueCopyBuffer = newscl::clEnqueueCopyBuffer;
    dispatch.clReleaseMemObject = newscl::clReleaseMemObject;
    dispatch.clReleaseProgram = newscl::clReleaseProgram;
    dispatch.clGetExtensionFunctionAddressForPlatform = newscl::clGetExtensionFunctionAddressForPlatform;
    dispatch.clGetKernelWorkGroupInfo= newscl::clGetKernelWorkGroupInfo;

    dispatch.clReleaseKernel = newscl::clReleaseKernel;
    dispatch.clReleaseEvent  = newscl::clReleaseEvent;
    dispatch.clReleaseCommandQueue = newscl::clReleaseCommandQueue;
    dispatch.clReleaseDevice = newscl::clReleaseDevice;
    dispatch.clReleaseContext= newscl::clReleaseContext;
    dispatch.clGetProgramBuildInfo= newscl::clGetProgramBuildInfo;
    is_init = true;
  }
  return &dispatch;
}
cl_context newscl::GetContext() {
  static _cl_context context;
  static bool is_init = false;
  if (!is_init) {
    context.dispatch = GetDispatch();
    is_init = true;
  }
  return &context;
}

cl_command_queue newscl::GetCommandQueue() {
  static _cl_command_queue queue;
  static bool is_init = false;
  if (!is_init) {
    queue.dispatch = GetDispatch();
    is_init = true;
  }
  return &queue;
}
cl_int newscl::clSetKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void *arg_value) {
  if (arg_index < 0 || arg_index >= kernel->is_pointer.size()) {
    return CL_INVALID_ARG_INDEX;
  }
  bool is_pointer = kernel->is_pointer[arg_index];
  if (is_pointer) {
    if (arg_value == NULL) {
      kernel->changed_mem_id.push_back({arg_index, 2147483247});
    } else {
      cl_mem mem = *(static_cast<cl_mem*>(const_cast<void*>(arg_value)));
      //fprintf(stderr, "Arg index:%d mem_id:%d kernel_id:%d\n", arg_index, mem->id, kernel->id);
      kernel->changed_mem_id.push_back({arg_index, mem->id});
    }
    return CL_SUCCESS;
  } else {
    //fprintf(stderr, "Arg index:%d size:%ld kernel_id:%d data:", arg_index, arg_size, kernel->id);
   // const char *targ_value = (const char*)(arg_value);
    //for(int i = 0; i < arg_size; ++i){
    //  fprintf(stderr, "%d ", targ_value[i]);
    //}
    //fprintf(stderr, "\n");
    kernel->const_data_info.push_back({(unsigned char)(arg_index), arg_size});
    std::copy(reinterpret_cast<const char*>(arg_value), reinterpret_cast<const char*>(arg_value) + arg_size, std::back_inserter(kernel->const_data));
    return CL_SUCCESS;
  }
}

cl_int newscl::clGetMemObjectInfo(cl_mem memobj,
                                  cl_mem_info param_name,
                                  size_t param_value_size,               
                                  void *param_value,
                                  size_t *param_value_size_ret) {

  if (param_value != NULL) {
    if (param_name == CL_MEM_TYPE) {
      *static_cast<cl_mem_object_type*>(param_value) = memobj->type;
    } else if (param_name == CL_MEM_FLAGS) {
      *static_cast<cl_mem_flags*>(param_value) = memobj->flags;
    } else if (param_name == CL_MEM_SIZE) {
      *static_cast<size_t*>(param_value) = memobj->size;
    } else if (param_name == CL_MEM_HOST_PTR) {
      param_value = memobj->host_ptr;
    } else if (param_name == CL_MEM_CONTEXT) {
      param_value = memobj->context;
    } else {
      assert(false);
    }
  }

  if(param_value_size_ret != NULL){
    if (param_name == CL_MEM_TYPE) {
      *param_value_size_ret = sizeof(cl_mem_object_type); 
    } else if (param_name == CL_MEM_FLAGS) {
      *param_value_size_ret = sizeof(cl_mem_flags);
    } else if (param_name == CL_MEM_SIZE) {
      *param_value_size_ret = sizeof(size_t);
    } else if (param_name == CL_MEM_HOST_PTR) {
      *param_value_size_ret = sizeof(void*);
    } else if (param_name == CL_MEM_CONTEXT) {
      *param_value_size_ret = sizeof(cl_context);
    } else {
      assert(false);
    }
  }

  return CL_SUCCESS;
}
cl_int newscl::clEnqueueWriteBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_write,
    size_t offset,
    size_t cb,
    const void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {
  if (command_queue != GetCommandQueue()) {
    return CL_INVALID_COMMAND_QUEUE;
  }
  //if (blocking_write == false) {
    //printf("non-blocking_write\n");
  //}
  if (num_events_in_wait_list != 0) {
    fprintf(stderr, "we do not support event\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
#ifdef DEBUG_
  fprintf(stderr, "write mem_id:%d size:%ld offset:%ld cb:%ld\n", buffer->id, buffer->size, offset, cb);
#endif //DEBUG_
  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueWriteBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&offset),
        sizeof(offset));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&cb),
        sizeof(cb));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&buffer->id),
        sizeof(buffer->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&blocking_write),
        sizeof(blocking_write));
  GetConnection()->Send(send_buf, kMsgMaxSize);
  GetConnection()->Send(reinterpret_cast<char*>(const_cast<void*>(ptr)), cb);

  if (blocking_write) {
    GetConnection()->Recv(recv_buf, kMsgMaxSize);
    return *(reinterpret_cast<cl_int*>(recv_buf));
  } else {
    return CL_SUCCESS;
  }
}
struct CatchData {
  newscl::SocketIOClient *conn_;
  int size_;
  char *dest_;
  CatchData(newscl::SocketIOClient *conn,
            int size,
            char *dest) : conn_(conn), size_(size), dest_(dest) {
  }
};
void* catcher(void *data) {
  newscl::SocketIOClient *conn = (reinterpret_cast<CatchData*>(data))->conn_;
  CatchData *catch_data = reinterpret_cast<CatchData*>(data);
  int size = (reinterpret_cast<CatchData*>(data))->size_;
  char *dest = (reinterpret_cast<CatchData*>(data))->dest_;
  bool ack = true;
  conn->Recv(dest, size);
  //fprintf(stderr, "recv OK");
  conn->Send(reinterpret_cast<char*>(&ack), sizeof(ack));
  delete catch_data;
  return NULL;
}

cl_int newscl::clEnqueueReadBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_read,
    size_t offset,
    size_t cb,
    void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {
  if (command_queue != GetCommandQueue()) {
    return CL_INVALID_COMMAND_QUEUE;
  }
  //if (blocking_read == false) {
    //printf("non-blocking_read\n");
  //}
  if (num_events_in_wait_list != 0) {
    fprintf(stderr, "we do not support event\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
#ifdef DEBUG_
  fprintf(stderr, "read mem_id:%d size:%ld offset:%ld cb:%ld\n", buffer->id, buffer->size, offset, cb);
#endif //DEBUG_
  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueReadBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf, &buf_offset, static_cast<void*>(&offset), sizeof(offset));
  Paste(send_buf, &buf_offset, static_cast<void*>(&cb), sizeof(cb));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&buffer->id),
        sizeof(buffer->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&blocking_read),
        sizeof(cl_bool));
  GetConnection()->Send(send_buf, kMsgMaxSize);

  if (blocking_read) {
    //GetConnection()->Recv(recv_buf, kMsgMaxSize);
    GetNonBlockingConnection()->Recv(reinterpret_cast<char*>(ptr), cb);
    //return *(reinterpret_cast<cl_int*>(recv_buf));
    return CL_SUCCESS;
  } else {
    //pthread_t *pid = new pthread_t();  // memory leak
    pthread_t pid;
    CatchData *catch_data = new CatchData(GetNonBlockingConnection(),
                                          cb,
                                          reinterpret_cast<char*>(ptr));
    if (pthread_create(&pid,
                       NULL,
                       catcher,
                       static_cast<void*>(catch_data)) != 0) {
      fprintf(stderr, "create thread failed\n");
    }
    return CL_SUCCESS;
  }
}
cl_int newscl::clBullet3ReadResultBack(void* data, cl_uint data_size) {
  fprintf(stderr, "HI data_size:%d\n", data_size);
  std::array<char, kMsgMaxSize> send_buf;
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kBullet3ReadResultBack;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  GetConnection()->Send(send_buf.data(), kMsgMaxSize);
  GetConnection()->Recv(static_cast<char*>(data), data_size);
  return CL_SUCCESS;
}
cl_int newscl::clBullet3RBPWriteToGPU(char* t_data,
                                     char* t_data_size,
                                     unsigned long long t_total_data_size,
                                     unsigned long long num_of_data) {
  unsigned long long *data_size = reinterpret_cast<unsigned long long*>(t_data_size);
  unsigned long long total_data_size  = 0;
  for (unsigned int i = 0; i < num_of_data; ++i, ++data_size)
    total_data_size += *data_size;
  assert(total_data_size == t_total_data_size);
  fprintf(stderr, "total data_size:%lld\n", total_data_size);
  std::array<char, kMsgMaxSize> send_buf;
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kBullet3RBPWriteToGPU;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&t_total_data_size),
        sizeof(t_total_data_size));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&num_of_data),
        sizeof(num_of_data));
  if (t_total_data_size) {
    GetConnection()->Send(send_buf.data(), kMsgMaxSize);
    GetConnection()->Send(t_data, t_total_data_size);
    GetConnection()->Send(t_data_size, sizeof(unsigned long long) * num_of_data);
  }
  return CL_SUCCESS;
}
cl_int newscl::clBullet3BPWriteToGPU(char* t_data,
                                     char* t_data_size,
                                     unsigned long long t_total_data_size,
                                     unsigned long long num_of_data) {
  unsigned long long *data_size = reinterpret_cast<unsigned long long*>(t_data_size);
  unsigned long long total_data_size  = 0;
  for (unsigned int i = 0; i < num_of_data; ++i, ++data_size)
    total_data_size += *data_size;
  assert(total_data_size == t_total_data_size);
  fprintf(stderr, "total data_size:%lld\n", total_data_size);
  std::array<char, kMsgMaxSize> send_buf;
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kBullet3BPWriteToGPU;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&t_total_data_size),
        sizeof(t_total_data_size));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&num_of_data),
        sizeof(num_of_data));
  if (t_total_data_size)
  {
    GetConnection()->Send(send_buf.data(), kMsgMaxSize);
    GetConnection()->Send(t_data, t_total_data_size);
    GetConnection()->Send(t_data_size, sizeof(unsigned long long) * num_of_data);
  }
  return CL_SUCCESS;
}
cl_int newscl::clBullet3NPWriteToGPU(char* t_data,
                                     char* t_data_size,
                                     unsigned long long t_total_data_size,
                                     unsigned long long num_of_data,
                                     unsigned long long num_of_bodies) {
  unsigned long long *data_size = reinterpret_cast<unsigned long long*>(t_data_size);
  unsigned long long total_data_size  = 0;
  for (unsigned int i = 0; i < num_of_data; ++i, ++data_size)
    total_data_size += *data_size;
  assert(total_data_size == t_total_data_size);
  fprintf(stderr, "total data_size:%lld\n", total_data_size);
  std::array<char, kMsgMaxSize> send_buf;
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kBullet3NPWriteToGPU;
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&t_total_data_size),
        sizeof(t_total_data_size));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&num_of_data),
        sizeof(num_of_data));
  Paste(send_buf.data(),
        &buf_offset,
        static_cast<void*>(&num_of_bodies),
        sizeof(num_of_bodies));
  if (t_total_data_size) {
    GetConnection()->Send(send_buf.data(), kMsgMaxSize);
    GetConnection()->Send(t_data, t_total_data_size);
    GetConnection()->Send(t_data_size, sizeof(unsigned long long) * num_of_data);
  }
  return CL_SUCCESS;
}
cl_mem newscl::clCreateBuffer(cl_context   context,
    cl_mem_flags flags,
    size_t       size,
    void *       host_ptr,
    cl_int *     errcode_ret) {
  if (context != GetContext()) {
    *errcode_ret = CL_INVALID_CONTEXT;
  }
  if (flags & CL_MEM_ALLOC_HOST_PTR == CL_MEM_USE_HOST_PTR) {
    *errcode_ret = CL_INVALID_VALUE;
  }
  static unsigned int mem_id = 0;
  cl_mem mem = new _cl_mem;
  mem->context = context;
  mem->flags = flags;

  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kCreateBuffer;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf, &buf_offset, static_cast<void*>(&flags), sizeof(flags));
  Paste(send_buf, &buf_offset, static_cast<void*>(&size), sizeof(size));
  GetConnection()->Send(send_buf, kMsgMaxSize);
  if ((flags & CL_MEM_COPY_HOST_PTR) == CL_MEM_COPY_HOST_PTR) {
    //mem->data = new char[size];
    //memcpy(mem->data, host_ptr, size);
    GetConnection()->Send(reinterpret_cast<const char *>(host_ptr), size);
    //mem->host_ptr = host_ptr;
  }

  //GetConnection()->Recv(recv_buf, kMsgMaxSize);
  //*errcode_ret = *(reinterpret_cast<cl_int*>(recv_buf));
  if(errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;

  mem->dispatch = GetDispatch();
  //mem->id = *(reinterpret_cast<unsigned int*>(recv_buf + sizeof(*errcode_ret)));
  mem->id = mem_id++;
  mem->type = CL_MEM_OBJECT_BUFFER;
  mem->size = size;
  mem->flags = flags;
  mem->context = GetContext();
#ifdef DEBUG_
  if ( host_ptr == NULL) {
    fprintf(stderr, "create mem_id:%d size:%ld\n", mem->id, size);
  } else {
    fprintf(stderr, "create size:%ld have data transfer\n", size);
  }
#endif //DEBUG_
  return mem;
}

cl_int newscl::clEnqueueTask(cl_command_queue command_queue,
                             cl_kernel kernel,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list,
                             cl_event *event) {
  if (command_queue != GetCommandQueue()) {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (kernel->program->context != GetContext()) {
    return CL_INVALID_KERNEL;
  }
  if (num_events_in_wait_list > 0) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
  if (event != NULL) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
#ifdef DEBUG_
  fprintf(stderr, "EnqueueTask kernel_id:%d\n", kernel->id);
#endif //DEBUG_
  int errcode_ret;
  char send_buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueTask;
  bzero(send_buf, kMsgMaxSize);
  //printf("kernel_id: %d\n", kernel->id);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(kernel->id)),
        sizeof(kernel->id));
  GetConnection()->Send(send_buf, kMsgMaxSize, true);

  return CL_SUCCESS;
}

cl_int newscl::clEnqueueNDRangeKernel(cl_command_queue command_queue,
                                      cl_kernel kernel,
                                      cl_uint work_dim,
                                      const size_t *global_work_offset,
                                      const size_t *global_work_size,
                                      const size_t *local_work_size,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event *event_wait_list,
                                      cl_event *event) {
  size_t *tlocal_work_size = const_cast<size_t*>(local_work_size);
  if (kernel->program->context != GetContext()) {
    return CL_INVALID_KERNEL;
  }
  if (num_events_in_wait_list > 0) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    return CL_INVALID_EVENT_WAIT_LIST;
  }
  if (event != NULL) {
    fprintf(stderr, "Sorry, we do not support event functionality\n");
    //return CL_INVALID_EVENT_WAIT_LIST;
  }
  if (local_work_size == NULL) {
    tlocal_work_size = new size_t[work_dim];
    bzero(tlocal_work_size, sizeof(size_t) * work_dim);
  }
#ifdef DEBUG_
  fprintf(stderr, "EnqueueNDRKernel kernel_id:%d\n", kernel->id);
#endif //DEBUG_

  int errcode_ret;
  char send_buf[kMsgMaxSize]={}, recv_buf[kMsgMaxSize]={};
  bzero(send_buf, kMsgMaxSize);
  size_t buf_offset = 0;
  int msg_type_num = newscl::cn_msg_type::kEnqueueNDRangeKernel;
  unsigned char t_work_dim = work_dim;
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(kernel->id)),
        sizeof(kernel->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(t_work_dim)),
        sizeof(t_work_dim));
  for (int i = 0; i < work_dim; ++i) {
    Paste(send_buf,
          &buf_offset,
          static_cast<void*>(const_cast<size_t*>(&(global_work_size[i]))),
          sizeof(global_work_size[i]));
  }
  buf_offset += (3 - work_dim) * sizeof(global_work_size[0]);

  for (int i = 0; i < work_dim; ++i) {
    Paste(send_buf,
          &buf_offset,
          static_cast<void*>(const_cast<size_t*>(&(tlocal_work_size[i]))),
          sizeof(tlocal_work_size[i]));
  }
  buf_offset += (3 - work_dim) * sizeof(tlocal_work_size[0]);
  unsigned char num_of_changed_num_id = kernel->changed_mem_id.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&num_of_changed_num_id),
        sizeof(num_of_changed_num_id));
  unsigned char num_of_const_arg = kernel->const_data_info.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&num_of_const_arg),
        sizeof(num_of_const_arg));
  unsigned int const_data_size = kernel->const_data.size();
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(const_data_size)),
        sizeof(const_data_size));

  GetConnection()->Send(send_buf, kMsgMaxSize, true);

  char arg_send_buf[kMsgMaxSize];
  const int max_mem_in_msg = kMsgMaxSize /
                             (sizeof(unsigned char) + sizeof(unsigned int));
  const int mem_send_times = (kernel->changed_mem_id.size() + max_mem_in_msg - 1) / max_mem_in_msg;
  //assert(mem_send_times<2);
  for (int i = 0; i < mem_send_times; ++i) {
    size_t arg_buf_offset = 0;
    const int end_j = 
      (i == mem_send_times - 1?kernel->changed_mem_id.size() : (i + 1) * max_mem_in_msg);
    for (int j = i * max_mem_in_msg; j < end_j ; ++j) {
      Paste(arg_send_buf,
            &arg_buf_offset,
            static_cast<void*>(&(kernel->changed_mem_id[j].first)),
            sizeof(kernel->changed_mem_id[j].first));
      Paste(arg_send_buf,
            &arg_buf_offset,
            static_cast<void*>(&(kernel->changed_mem_id[j].second)),
            sizeof(kernel->changed_mem_id[j].second));
    }
    GetConnection()->Send(arg_send_buf, kMsgMaxSize, true);
  }

  if (const_data_size > 0) {
    GetConnection()->Send(kernel->const_data.data(), kernel->const_data.size(), true);
  }

  char const_send_buf[kMsgMaxSize];
  size_t const_buf_offset = 0;

  const int kMaxConstInfoInMsg = kMsgMaxSize /
                                    (sizeof(unsigned char) + sizeof(size_t));
  const int kConstSendTimes = ( num_of_const_arg + kMaxConstInfoInMsg - 1)
                              / kMaxConstInfoInMsg;
  //assert(kConstSendTimes<2);
  //if(kConstSendTimes>1 && kernel->kernel_name == "flipFloatKernel"){
  //  int a = 2;
  //  a++;
  //}
  for (int i = 0; i < kConstSendTimes; ++i) {
    const int kEndJ = 
      (i == kConstSendTimes - 1?kernel->const_data_info.size() : (i + 1) * kMaxConstInfoInMsg);
    for (int j = i * kMaxConstInfoInMsg; j < kEndJ ; ++j) {
      Paste(const_send_buf,
            &const_buf_offset,
            static_cast<void*>(&(kernel->const_data_info[j].arg_idx)),
            sizeof(kernel->const_data_info[j].arg_idx));
      Paste(const_send_buf,
            &const_buf_offset,
            static_cast<void*>(&(kernel->const_data_info[j].size)),
            sizeof(kernel->const_data_info[j].size));
    }
    GetConnection()->Send(const_send_buf, kMsgMaxSize, true);
  }
  //fprintf(stderr, "const_data_size:%d, memSendTime:%d, kConstSendTime:%d\n", const_data_size, mem_send_times, kConstSendTimes);

  //GetConnection()->Recv(recv_buf, kMsgMaxSize);
  //errcode_ret = *(reinterpret_cast<cl_int*>(recv_buf));

  kernel->changed_mem_id.clear();
  kernel->const_data_info.clear();
  kernel->const_data.clear();
  if (local_work_size == NULL) {
    delete[] tlocal_work_size;
  }
  return CL_SUCCESS;
}

cl_command_queue newscl::clCreateCommandQueue(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int *errcode_ret) {
  if (context != GetContext()) {
    *errcode_ret = CL_INVALID_CONTEXT;
    return NULL;
  }
  if (device != GetDevice()) {
    *errcode_ret = CL_INVALID_DEVICE;
    return NULL;
  }
  if (errcode_ret != NULL) {
    *errcode_ret = CL_SUCCESS;
  }
  return GetCommandQueue();
}

cl_program newscl::clCreateProgramWithSource(
    cl_context       context,
    cl_uint           count ,
    const char **     strings ,
    const size_t *    lengths ,
    cl_int *          errcode_ret) {
  static int id = 0;
  ++id;
  cl_program p_program = new _cl_program;
  p_program ->dispatch = GetDispatch();
  p_program -> context = context;
  int msg_type_num = cn_msg_type::kCreateProgramWithSource;

  if (context != GetContext()) {
    *errcode_ret = CL_INVALID_CONTEXT;
    return NULL;
  }

  char buf[kMsgMaxSize], recv_buf[kMsgMaxSize];
  int buf_offset = 0;
  bzero(buf, kMsgMaxSize);
  memcpy(buf + buf_offset,
         reinterpret_cast<char*>(&msg_type_num),
         sizeof(msg_type_num));
  buf_offset += sizeof(msg_type_num);
  memcpy(buf + buf_offset, reinterpret_cast<char*>(&count), sizeof(count));
  buf_offset += sizeof(count);
  memcpy(buf + buf_offset, reinterpret_cast<char*>(&id), sizeof(id));
  buf_offset += sizeof(count);
  GetConnection()->Send(buf, kMsgMaxSize);

  size_t *send_lengths = new size_t[count];

  p_program->code = "";
  for (int i = 0; i < count; ++i) {
    if (lengths == NULL || lengths[i] == 0)
    {
      send_lengths[i] = strlen(strings[0]);
    } else
    {
      send_lengths[i] = lengths[i];
    }
    GetConnection()->Send(
        reinterpret_cast<char*>(const_cast<size_t*>(&send_lengths[i])),
        sizeof(size_t));
  }
  //printf("strings[i]:%s\n", strings[0]);
  for (int i = 0; i < count; ++i) {
    GetConnection()->Send(strings[i], send_lengths[i]);
    p_program->code += "\n";
    p_program->code += std::string(strings[i], send_lengths[i]);
  }
  newscl::Preprocessor(p_program->code);
  GetConnection()->Recv(recv_buf, kMsgMaxSize);
  p_program->id = *reinterpret_cast<unsigned int*>(recv_buf);
  if(errcode_ret){
    *errcode_ret = *reinterpret_cast<cl_int*>(recv_buf + sizeof(p_program->id));
  }
  delete [] send_lengths;
  return p_program;
}

cl_kernel newscl::clCreateKernel(cl_program  program,
                                 const char *kernel_name,
                                 cl_int *errcode_ret) {
  char send_buf[kMsgMaxSize];
  char recv_buf[kMsgMaxSize];
  int msg_type_num = cn_msg_type::kCreateKernel;
  size_t buf_offset = 0;
  size_t kernel_name_length = strlen(kernel_name);
  _cl_kernel *kernel = new _cl_kernel;
  kernel->program = program;
  kernel->dispatch = GetDispatch();
  kernel->kernel_name = std::string(kernel_name);

  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&(program->id)),
        sizeof(program->id));
  Paste(send_buf,
        &buf_offset,
        static_cast<void*>(&kernel_name_length),
        sizeof(kernel_name_length));

  GetConnection()->Send(send_buf, kMsgMaxSize);
  GetConnection()->Send(kernel_name, kernel_name_length);
  GetConnection()->Recv(recv_buf, kMsgMaxSize);

  cl_int actual_errcode_ret = *(reinterpret_cast<cl_int*>(recv_buf));
  if (errcode_ret != NULL)
    *errcode_ret = actual_errcode_ret;

  sscanf(recv_buf + sizeof(cl_int), "kernel id:%d", &(kernel->id));
  if (actual_errcode_ret == CL_SUCCESS) {
    std::vector<std::string> arg_list;
    newscl::FindFuncArgList(program->code, kernel_name, arg_list);
    if (arg_list.size() > 127) {
      perror("The length of parameter list cannot excceed 127");
    }
    kernel->is_pointer.resize(arg_list.size());
    for (int i = 0; i < kernel->is_pointer.size(); ++i) {
      kernel->is_pointer[i] = newscl::IsPointer(arg_list[i]);
    }
  }
  return kernel;
}
cl_int newscl::clBuildProgram(cl_program program,
                              cl_uint num_devices,
                              const cl_device_id *device_list,
                              const char *options,
                              void (*pfn_notify)(cl_program, void *user_data),
                              void *user_data) {
  // disabled options, and pfn_notify and user_data
  char send_buf[kMsgMaxSize];
  char recv_buf[kMsgMaxSize];
  int msg_type_num = cn_msg_type::kBuildProgram;
  size_t buf_offset = 0;
  int error_code;
  if (device_list != NULL &&(num_devices != 1 || device_list[0] != GetDevice())) {
    return CL_INVALID_DEVICE;
  }
  unsigned int options_leng = options != NULL ? strlen(options) : 0;
  
  bzero(send_buf, kMsgMaxSize);
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&msg_type_num),
        sizeof(msg_type_num));
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&(program->id)),
        sizeof(num_devices));
  Paste(send_buf,
        &buf_offset,
        reinterpret_cast<void*>(&(options_leng)),
        sizeof(options_leng));
  GetConnection()->Send(send_buf, kMsgMaxSize);
  if (options_leng != 0)
  {
    GetConnection()->Send(options, options_leng);
  }
  GetConnection()->Recv(recv_buf, kMsgMaxSize);
  error_code = *(reinterpret_cast<int*>(recv_buf));
  return error_code;
}
cl_context newscl::clCreateContext(
    const cl_context_properties* /* properties */,
    cl_uint                  num_devices ,
    const cl_device_id*     devices ,
    void(CL_CALLBACK*)(
      const char*,
      const void*,
      size_t,
      void*),
    void*                  /* user_data */,
    cl_int*                 errcode_ret ) {
  if (num_devices != 1 || *devices != GetDevice()) {
    *errcode_ret = CL_INVALID_DEVICE;
    return NULL;
  }
  if (errcode_ret != NULL)
  {
    *errcode_ret = CL_SUCCESS;
  }

  return GetContext();
}

newscl::SocketIOClient* newscl::GetDispatcherConnection() {
  static SocketIOClient conn(newscl::kDispatcherIP, newscl::kDispatcherPort);
  static bool is_init = false;
  if (!is_init) {
    conn.Connect();
    is_init = true;
  }
  return &conn;
}

cl_platform_id newscl::GetPlatform() {
  #ifdef DEBUG
    fprintf(stderr, "GetPlatform()\n");
  #endif
  static _cl_platform_id platform;
  // static _cl_icd_dispatch dispatch;
  static bool is_init = false;
  bool accept;
  if (!is_init) {
    std::array<char, kMsgMaxSize> send_buf;
    std::array<char, kMsgMaxSize> recv_buf;
    size_t buf_offset = 0;
    unsigned long int client_id = -1;
    {
      boost::property_tree::ptree pt;
      try
      {
        boost::property_tree::json_parser::read_json("/opt/client.json", pt);
      } catch(...)
      {

      }
      client_id  = pt.get<decltype(client_id)>("id");
    }

    int msg_type_num = dispatcher_msg_type::kGetBindingCN;
    Paste(send_buf.data(),
          &buf_offset,
          static_cast<void*>(&msg_type_num),
          sizeof(msg_type_num));
    Paste(send_buf.data(),
          &buf_offset,
          static_cast<void*>(&client_id),
          sizeof(client_id));
//    GetDispatcherConnection()->Send(send_buf.data(), send_buf.size());
//    GetDispatcherConnection()->Recv(recv_buf.data(), recv_buf.size());
    double client_weight = *(reinterpret_cast<double*>(recv_buf.data()));
//    std::string ip{recv_buf.data() + sizeof(client_weight)};
    //fprintf(stderr, "cn_ip:%s\n", ip.c_str());
    //fprintf(stderr, "app weight:%lf\n", client_weight);
//    auto idx = ip.find(' ');
//    if (idx != std::string::npos) {
//      ip.erase(ip.begin() + ip.find(' '), ip.end());
//    }
    //fprintf(stderr, "get connection()\n");
    GetConnection(0, "140.112.28.114");  // connection init
    //fprintf(stderr, "get non_blocking connection()\n");
    GetNonBlockingConnection(0, "140.112.28.114");  // connection init
    //fprintf(stderr, "get finish connection()\n");
    GetFinishConnection(0, "140.112.28.114");  // connection init
    //fprintf(stderr, "get connection() end\n");
    //GetDataConnection(ip);  // connection init

    platform.dispatch = GetDispatch();
    msg_type_num = cn_msg_type::kReg;

    send_buf.fill(0);
    recv_buf.fill(0);
    buf_offset = 0;
    Paste(send_buf.data(), &buf_offset, &msg_type_num, sizeof(msg_type_num));
    Paste(send_buf.data(), &buf_offset, &client_weight, sizeof(client_weight));
    GetConnection(0)->Send(send_buf.data(), send_buf.size());
    GetConnection(0)->Recv(recv_buf.data(), send_buf.size());
    //fprintf(stderr, "reg echo: %s\n", recv_buf.data());

    is_init = true;


    //fprintf(stderr, "get connection()\n");
    GetConnection(0, "192.168.48.247");  // connection init
    //fprintf(stderr, "get non_blocking connection()\n");
    GetNonBlockingConnection(0, "192.168.48.247");  // connection init
    //fprintf(stderr, "get finish connection()\n");
    GetFinishConnection(0, "192.168.48.247");  // connection init
    //fprintf(stderr, "get connection() end\n");

    GetConnection(1)->Send(send_buf.data(), send_buf.size());
    GetConnection(1)->Recv(recv_buf.data(), send_buf.size());

  }
  return &platform;
}

newscl::SocketIOClient* newscl::GetFinishConnection2(const std::string &ip) {
  static SocketIOClient conn(ip, newscl::kCNFinishPort);
  static bool is_init = false;
  if (!is_init) {
    conn.Connect();
    is_init = true;
    int flag = 1;
    int result = setsockopt(conn.sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
    assert(result>=0);
  }
  return &conn;
}

newscl::SocketIOClient* newscl::GetFinishConnection(int no, const std::string &ip) {
  static std::vector<SocketIOClient> connVector;
  if(ip.size() > 0) {
    SocketIOClient *conn = new SocketIOClient(ip, newscl::kCNFinishPort);
    conn->Connect();
    int flag = 1;
    int result = setsockopt(conn->sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
      //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}

newscl::SocketIOClient* newscl::GetConnection(int no, const std::string &ip) {
  static std::vector<SocketIOClient> connVector;
  if(ip.size() > 0) {
    SocketIOClient *conn = new SocketIOClient(ip, newscl::kCNPort);
    conn->Connect();
    int flag = 1;
    int result = setsockopt(conn->sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
      //assert(result>=0);
    if(result >= 0){
      connVector.push_back(*conn);
    }
  }
  if(no < connVector.size()){
    return &(connVector[no]);
  } else {
    return NULL;
  }
}

newscl::SocketIOClient* newscl::GetConnection2(const std::string &ip) {
  static SocketIOClient conn(ip, newscl::kCNPort);
  static bool is_init = false;
  if (!is_init) {
    conn.Connect();
    is_init = true;
    int flag = 1;
    int result = setsockopt(conn.sock(),
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char*) &flag,
                            sizeof(int));
    assert(result>=0);
  }
  return &conn;
}


cl_device_id newscl::GetPhysicalDevicesInfo(unsigned int dev_type) {
  /*
  static struct _cl_device_id dev;
  static bool isInit = false;
  if(!isInit){
    //boost::array<char, 32> tmp;
    cl_uint id;
    unsigned int msgType = MSG_TYPE::getDevice;
    //memcpy(&tmp.front(), id, sizeof(unsigned int))
    getConnection()->send(reinterpret_cast<char*>(&msgType),
                          sizeof(unsigned int));
    getConnection()->send(reinterpret_cast<char*>(&devType),
                          sizeof(unsigned int));
    getConnection()->recv(reinterpret_cast<char*>(&id), sizeof(cl_uint));
    printf("id: %d\n", id);
    dev.dispatch = getDispatch();
    isInit = true;
  }
  return &dev;
  */
}
cl_device_id newscl::GetDevice() {
  static struct _cl_device_id dev;
  static bool is_init = false;
  if (!is_init) {
    dev.dispatch = GetDispatch();
    is_init = true;
  }
  return &dev;
}
cl_int newscl::clGetDeviceIDs(
    cl_platform_id platform,
    cl_device_type device_type,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices) {
  if (platform != newscl::GetPlatform()) {
    return CL_INVALID_PLATFORM;
  }
  if (num_entries && !devices || !num_entries && devices) {
    return CL_INVALID_VALUE;
  }
  if (num_entries) {
    devices[0] = GetDevice();
  }
  if (num_devices) {
    *num_devices = 1;
  }
  return CL_SUCCESS;
}

cl_int newscl::clGetDeviceInfo(
    cl_device_id device,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret) {
  if (device != GetDevice()) {
    return CL_INVALID_DEVICE;
  }
  switch (param_name) {
    case CL_DEVICE_NAME: {
      snprintf(static_cast<char*>(param_value),
               param_value_size,
               "virtual device");
      break;
    }
    case CL_DEVICE_VENDOR: {
      snprintf(static_cast<char*>(param_value), param_value_size, "NEWSLAB");
      break;
    }
    case CL_DEVICE_VERSION: {
      snprintf(static_cast<char*>(param_value), param_value_size, "NEWSCL 1.0");
      break;
    }
    case CL_DEVICE_TYPE: {
      *(static_cast<cl_device_type *>(param_value)) = CL_DEVICE_TYPE_GPU;
      break;
    }
    case CL_DEVICE_MAX_COMPUTE_UNITS: {
      *(static_cast<cl_uint *>(param_value)) = 12;
      break;
    }
    case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: {
      *(static_cast<cl_uint *>(param_value)) = 3;
      break;
    }
    case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
      *(static_cast<size_t *>(param_value)) = 256;
      *(static_cast<size_t *>(param_value) + 1) = 256;
      *(static_cast<size_t *>(param_value) + 2) = 256;
      break;
    }
    case CL_DEVICE_MAX_WORK_GROUP_SIZE: {
      *(static_cast<size_t *>(param_value)) = 256;
      break;
    }
    case CL_DEVICE_MAX_CLOCK_FREQUENCY: {
      *(static_cast<cl_uint *>(param_value)) = 790;
      break;
    }
    case CL_DEVICE_ADDRESS_BITS: {
      *(static_cast<cl_uint *>(param_value)) = 32;
      break;
    }
    case CL_DEVICE_MAX_MEM_ALLOC_SIZE: {
      *(static_cast<cl_ulong *>(param_value)) =  256 * 1024 * 1024;
      break;
    }
    case CL_DEVICE_GLOBAL_MEM_SIZE: {
      *(static_cast<cl_ulong *>(param_value)) = 512 * 1024 * 1024;
      break;
    }
    case CL_DEVICE_ERROR_CORRECTION_SUPPORT: {
      *(static_cast<cl_bool *>(param_value)) = false;
      break;
    }
    case CL_DEVICE_LOCAL_MEM_TYPE: {
      *(static_cast<cl_device_local_mem_type* >(param_value)) = CL_LOCAL;
      break;
    }
    case CL_DEVICE_LOCAL_MEM_SIZE: {
      *(static_cast<cl_ulong *>(param_value)) = 32 * 1024;
      break;
    }
    case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: {
      *(static_cast<cl_ulong *>(param_value)) = 64 * 1024;
      break;
    }
    case CL_DEVICE_QUEUE_PROPERTIES: {
      *(static_cast<cl_command_queue_properties*>(param_value)) =
        CL_QUEUE_PROFILING_ENABLE;
      break;
    }
    case CL_DEVICE_IMAGE_SUPPORT: {
      *(static_cast<cl_bool*>(param_value)) = true;
      break;
    }
    case CL_DEVICE_MAX_READ_IMAGE_ARGS: {
      *(static_cast<cl_uint*>(param_value)) = 128;
      break;
    }
    case CL_DEVICE_MAX_WRITE_IMAGE_ARGS: {
      *(static_cast<cl_uint*>(param_value)) = 8;
      break;
    }
    case CL_DEVICE_IMAGE2D_MAX_WIDTH: {
      *(static_cast<size_t*>(param_value)) = 16384;
      break;
    }
    case CL_DEVICE_IMAGE2D_MAX_HEIGHT: {
      *(static_cast<size_t*>(param_value)) = 16384;
      break;
    }
    case CL_DEVICE_IMAGE3D_MAX_WIDTH: {
      *(static_cast<size_t*>(param_value)) = 2048;
      break;
    }
    case CL_DEVICE_IMAGE3D_MAX_HEIGHT: {
      *(static_cast<size_t*>(param_value)) = 2048;
      break;
    }
    case CL_DEVICE_IMAGE3D_MAX_DEPTH: {
      *(static_cast<size_t*>(param_value)) = 2048;
      break;
    }
    case CL_DEVICE_EXTENSIONS: {
      snprintf(static_cast<char*>(param_value), param_value_size,
          "cl_khr_global_int32_base_atomics\
           cl_khr_global_int32_extended_atomics\
           cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics\
           cl_khr_3d_image_writes cl_khr_byte_addressable_store\
           cl_khr_gl_sharing cl_ext_atomic_counters_32\
           cl_amd_device_attribute_query cl_amd_vec3 cl_amd_printf\
           cl_amd_media_ops cl_amd_popcnt");
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: {
      *(static_cast<cl_uint*>(param_value)) = 16;
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: {
      *(static_cast<cl_uint*>(param_value)) = 8;
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: {
      *(static_cast<cl_uint*>(param_value)) = 4;
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG: {
      *(static_cast<cl_uint*>(param_value)) = 2;
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: {
      *(static_cast<cl_uint*>(param_value)) = 4;
      break;
    }
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: {
      *(static_cast<cl_uint*>(param_value)) = 0;
      break;
    }
  }
  return CL_SUCCESS;
}

cl_int newscl::clGetPlatformInfo(
    cl_platform_id   platform,
    cl_platform_info param_name,
    size_t           param_value_size,
    void *           param_value,
    size_t *         param_value_size_ret) {
  std::string str;
  bool trueParamName = false;

  if (newscl::GetPlatform() == platform) {
    switch (param_name) {
      case CL_PLATFORM_NAME: {
        str = newscl::kPlatformName;
        trueParamName = true;
        break;
      }
      case CL_PLATFORM_EXTENSIONS: {
        str = "cl_khr_icd";
        trueParamName = true;
        break;
      }
      case CL_PLATFORM_ICD_SUFFIX_KHR: {
        str = "NEWSCL";
        trueParamName = true;
        break;
      }

      case CL_PLATFORM_VERSION: {
        str = "OpenCL 1.1 NEWSCL";
        trueParamName = true;
        break;
      }
      case CL_PLATFORM_VENDOR: {
        str = "NEWSLAB";
        trueParamName = true;
        break;
      }
    }
    if (param_value) {
      if (param_value_size < str.size() + 1)
        return CL_INVALID_VALUE;
      strncpy(static_cast<char*>(param_value),
              str.c_str(),
              param_value_size - 1);
      static_cast<char*>(param_value)[str.size()] = '\0';
    }
    if (param_value_size_ret) {
      *param_value_size_ret = str.size() + 1;
    }
    if (!trueParamName) {
      return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
  } else {
    return CL_INVALID_PLATFORM;
  }
  return CL_INVALID_VALUE;
}
void* newscl::clGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                                      const char* funcName) {
  if (funcName == NULL) {
    return reinterpret_cast<void*>(clIcdGetPlatformIDsKHR);
  } else if (strcmp(funcName, "clBullet3ReadResultBack") == 0) {
    return reinterpret_cast<void*>(clBullet3ReadResultBack);
  } else if (strcmp(funcName, "clBullet3NPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3NPWriteToGPU);
  } else if (strcmp(funcName, "clBullet3RBPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3RBPWriteToGPU);
  } else if (strcmp(funcName, "clBullet3BPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3BPWriteToGPU);
  }
  return reinterpret_cast<void*>(clIcdGetPlatformIDsKHR);
}
void* newscl::clGetExtensionFunctionAddress(const char *funcName) {
  if (funcName == NULL) {
    return reinterpret_cast<void*>(clIcdGetPlatformIDsKHR);
  } else if (strcmp(funcName, "clBullet3ReadResultBack") == 0) {
    return reinterpret_cast<void*>(clBullet3ReadResultBack);
  } else if (strcmp(funcName, "clBullet3NPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3NPWriteToGPU);
  } else if (strcmp(funcName, "clBullet3RBPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3RBPWriteToGPU);
  } else if (strcmp(funcName, "clBullet3BPWriteToGPU") == 0) {
    return reinterpret_cast<void*>(clBullet3BPWriteToGPU);
  }
  return reinterpret_cast<void*>(clIcdGetPlatformIDsKHR);
}

cl_int newscl::clGetPlatformIDs(
    cl_uint          num_entries,
    cl_platform_id * platforms,
    cl_uint *        num_platforms) {
  if (num_entries == 0 && platforms) return CL_INVALID_VALUE;
  if (!num_platforms && !platforms) return CL_INVALID_VALUE;

  if (platforms) {
    platforms[0] = newscl::GetPlatform();
  }
  if (num_platforms) {
    *num_platforms = 1;
  }
  return CL_SUCCESS;
}


cl_int newscl::clGetKernelWorkGroupInfo(cl_kernel kernel,
                                cl_device_id device,
                                cl_kernel_work_group_info param_name,
                                size_t param_value_size,
                                void *param_value,
                                size_t *param_value_size_ret)
{
  if (device != GetDevice())
  {
    return CL_INVALID_DEVICE;
  }
  switch (param_name)
  {
    case CL_KERNEL_WORK_GROUP_SIZE:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = sizeof(size_t);
      if(param_value != NULL)
        *reinterpret_cast<size_t*>(param_value) = 1024;
      break;
    }
    case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
    {
      fprintf(stderr, "Sorry, we do not support!\n");
      break;
    }
    case CL_KERNEL_LOCAL_MEM_SIZE:
    {
      fprintf(stderr, "Sorry, we do not support!\n");
      break;
    }
    default:
    {
      return CL_INVALID_VALUE;
      break;
    }
  }
  return CL_SUCCESS;
}
cl_int newscl::clGetProgramBuildInfo(cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
  if (device != GetDevice()){
    return CL_INVALID_DEVICE;
  }
  switch (param_name)
  {
    case CL_PROGRAM_BUILD_STATUS:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = sizeof(cl_build_status);
      if(param_value != NULL)
        *reinterpret_cast<cl_build_status*>(param_value) = CL_BUILD_SUCCESS;
      break;
    }
    case CL_PROGRAM_BUILD_OPTIONS:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = 1;
      if(param_value != NULL)
        *reinterpret_cast<char*>(param_value) = '\0';
      break;
    }
    case CL_PROGRAM_BUILD_LOG:
    {
      if (param_value_size_ret != NULL)
        *param_value_size_ret = 1;
      if(param_value != NULL)
        *reinterpret_cast<char*>(param_value) = '\0';
      break;
    }
    default:
    {
      return CL_INVALID_VALUE;
    }
  }
  return CL_SUCCESS;
}

cl_int newscl::clReleaseKernel(cl_kernel kernel)
{
  return CL_SUCCESS;
}

cl_int newscl::clReleaseEvent(cl_event event)
{
  return CL_SUCCESS;
}

cl_int newscl::clReleaseCommandQueue(cl_command_queue command_queue)
{
  return CL_SUCCESS;
}

cl_int newscl::clReleaseDevice(cl_device_id device)
{
  return CL_SUCCESS;
}
cl_int newscl::clReleaseContext(cl_context context)
{
  return CL_SUCCESS;
}
int main() {
}
