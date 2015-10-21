// Copyright [2013] (JoenChen:joen@joen.cc)
#include "CL/cl.h"
#include "CL/cl_ext.h"
#include "./cl_dispatch.h"
#include "./cl_api.h"
#include "newscl.h"
#include "./runtime.h"

cl_int clFinish(cl_command_queue queue) {
  return newscl::clFinish(queue);
}

cl_int clGetContextInfo(
    cl_context         context,
    cl_context_info    param_name,
    size_t             param_value_size,
    void *             param_value,
    size_t *           param_value_size_ret) {
  return newscl::clGetContextInfo(context,
                                  param_name,
                                  param_value_size,
                                  param_value,
                                  param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem memobj,
                   cl_mem_info param_name,
                   size_t param_value_size,
                   void *param_value,
                   size_t *param_value_size_ret) {
  return newscl::clGetMemObjectInfo(memobj,
                                    param_name,
                                    param_value_size,
                                    param_value,
                                    param_value_size_ret);
}

CL_API_ENTRY cl_context CL_API_CALL clCreateContextFromType(
    cl_context_properties   *properties,
    cl_device_type  device_type,
    void(*pfn_notify) (const char *errinfo,
      const void  *private_info,
      size_t  cb,
      void  *user_data),
    void  *user_data,
    cl_int  *errcode_ret) {
  return newscl::clCreateContextFromType(properties,
                                         device_type,
                                         pfn_notify,
                                         user_data,
                                         errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clSetKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void *arg_value) {
  return newscl::clSetKernelArg(kernel, arg_index, arg_size, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueWriteBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_write,
    size_t offset,
    size_t cb,
    void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {
  return newscl::clEnqueueWriteBuffer(command_queue,
                                      buffer,
                                      blocking_write,
                                      offset,
                                      cb,
                                      ptr,
                                      num_events_in_wait_list,
                                      event_wait_list,
                                      event);
}
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReadBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_read,
    size_t offset,
    size_t cb,
    void *ptr,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event) {
  return newscl::clEnqueueReadBuffer(command_queue,
                                     buffer,
                                     blocking_read,
                                     offset,
                                     cb,
                                     ptr,
                                     num_events_in_wait_list,
                                     event_wait_list,
                                     event);
}
CL_API_ENTRY cl_int CL_API_CALL clEnqueueTask(
    cl_command_queue  command_queue,
    cl_kernel         kernel,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) {
  return newscl::clEnqueueTask(command_queue,
                               kernel,
                               num_events_in_wait_list,
                               event_wait_list,
                               event);
}

CL_API_ENTRY cl_int CL_API_CALL                                                     
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                              cl_kernel kernel,
                              cl_uint work_dim,
                              const size_t *global_work_offset,
                              const size_t *global_work_size,
                              const size_t *local_work_size,
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list,
                              cl_event *event) {
  return newscl::clEnqueueNDRangeKernel(command_queue,
                                        kernel,
                                        work_dim,
                                        global_work_offset,
                                        global_work_size,
                                        local_work_size,
                                        num_events_in_wait_list,
                                        event_wait_list,
                                        event);
}

CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueue(
    cl_context                     context,
    cl_device_id                   device,
    cl_command_queue_properties    properties,
    cl_int *                       errcode_ret) {
  return newscl::clCreateCommandQueue(context,
                                      device,
                                      properties,
                                      errcode_ret);
}

CL_API_ENTRY cl_kernel CL_API_CALL clCreateKernel(
    cl_program  program,
    const char *kernel_name,
    cl_int *errcode_ret) {
  return newscl::clCreateKernel(program, kernel_name, errcode_ret);
}


CL_API_ENTRY cl_mem clCreateBuffer(
    cl_context   context,
    cl_mem_flags flags,
    size_t       size,
    void *       host_ptr,
    cl_int *     errcode_ret) {
  return newscl::clCreateBuffer(context, flags, size, host_ptr, errcode_ret);
}


CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithSource(
    cl_context       context,
    cl_uint           count ,
    const char **     strings ,
    const size_t *    lengths ,
    cl_int *          errcode_ret) {
  return newscl::clCreateProgramWithSource(context,
                                           count,
                                           strings,
                                           lengths,
                                           errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clIcdGetPlatformIDsKHR(
    cl_uint              num_entries,
    cl_platform_id      *platforms,
    cl_uint             *num_platforms) {
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

CL_API_ENTRY void* CL_API_CALL clGetExtensionFunctionAddress(
    const char *funcName) {
  return newscl::clGetExtensionFunctionAddress(funcName);
}

CL_API_ENTRY cl_context CL_API_CALL clCreateContext(
    const cl_context_properties * properties,
    cl_uint                 num_devices,
    const cl_device_id *    devices,
    void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
    void *                  user_data,
    cl_int *                errcode_ret) {
  return newscl::clCreateContext(properties,
                                 num_devices,
                                 devices,
                                 pfn_notify,
                                 user_data,
                                 errcode_ret);
}
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDs(
    cl_platform_id platform,
    cl_device_type device_type,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices) {
  return newscl::clGetDeviceIDs(platform,
                                device_type,
                                num_entries,
                                devices,
                                num_devices);
}
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceInfo(
    cl_device_id device,
    cl_device_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret) {
  return newscl::clGetDeviceInfo(device,
                                 param_name,
                                 param_value_size,
                                 param_value,
                                 param_value_size_ret);
}


CL_API_ENTRY cl_int CL_API_CALL clGetPlatformInfo(
    cl_platform_id   platform,
    cl_platform_info param_name,
    size_t           param_value_size,
    void *           param_value,
    size_t *         param_value_size_ret) {
  return newscl::clGetPlatformInfo(
      platform,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret);
}


CL_API_ENTRY cl_int CL_API_CALL clGetPlatformIDs(
    cl_uint          num_entries,
    cl_platform_id * platforms,
    cl_uint *        num_platforms) {
  return newscl::clGetPlatformIDs(num_entries, platforms, num_platforms);
}
CL_API_ENTRY cl_int CL_API_CALL clReleaseMemObject(cl_mem memobj) {
  return newscl::clReleaseMemObject(memobj);
}
