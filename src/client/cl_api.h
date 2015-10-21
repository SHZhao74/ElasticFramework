// Copyright [2013] (JoenChen:joen@joen.cc)
#ifndef NEWSCL_CLIENT_CL_API_H_
#define NEWSCL_CLIENT_CL_API_H_
#include "CL/opencl.h"
#include <utility>
#include <vector>
#include <map>
#include <string>
extern "C"{
CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                              cl_kernel kernel,
                              cl_uint work_dim,
                              const size_t *global_work_offset,
                              const size_t *global_work_size,
                              const size_t *local_work_size,
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list,
                              cl_event *event) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem, cl_mem_info, size_t, void *, size_t*)
CL_API_SUFFIX__VERSION_1_0;
                              
CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         /* context */, 
		cl_context_info    /* param_name */, 
		size_t             /* param_value_size */, 
		void *             /* param_value */, 
		size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         /* context */, 
		cl_context_info    /* param_name */, 
		size_t             /* param_value_size */, 
		void *             /* param_value */, 
		size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties * /* properties */,
		cl_device_type          /* device_type */,
		void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
		void *                  /* user_data */,
		cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;


CL_API_ENTRY cl_int CL_API_CALL clSetKernelArg (	cl_kernel kernel,
		cl_uint arg_index,
		size_t arg_size,
		const void *arg_value);

CL_API_ENTRY cl_int CL_API_CALL clEnqueueWriteBuffer (	cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_write,
		size_t offset,
		size_t cb,
		const void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event);

CL_API_ENTRY cl_int CL_API_CALL clEnqueueReadBuffer (	cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_read,
		size_t offset,
		size_t cb,
		void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event);

CL_API_ENTRY cl_int CL_API_CALL clEnqueueTask(cl_command_queue  /* command_queue */,
		cl_kernel         /* kernel */,
		cl_uint           /* num_events_in_wait_list */,
		const cl_event *  /* event_wait_list */,
		cl_event *        /* event */);


CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     /* context */, 
		cl_device_id                   /* device */, 
		cl_command_queue_properties    /* properties */,
		cl_int *                       /* errcode_ret */);

CL_API_ENTRY cl_kernel CL_API_CALL clCreateKernel (cl_program  program,
		const char *kernel_name,
		cl_int *errcode_ret);

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithSource(cl_context        /* context */,
		cl_uint           /* count */,
		const char **     /* strings */,
		const size_t *    /* lengths */,						  
		cl_int *          /* errcode_ret */);

CL_API_ENTRY cl_int CL_API_CALL clBuildProgram(cl_program           /* program */,
		cl_uint              /* num_devices */,
		const cl_device_id * /* device_list */,
		const char *         /* options */, 
		void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
		void *               /* user_data */);


CL_API_ENTRY cl_context CL_API_CALL clCreateContext(const cl_context_properties * /* properties */,
		cl_uint                 /* num_devices */,
		const cl_device_id *    /* devices */,
		void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
		void *                  /* user_data */,
		cl_int *                /* errcode_ret */);

CL_API_ENTRY cl_int  CL_API_CALL clIcdGetPlatformIDsKHR(cl_uint          num_entries,
		cl_platform_id  *platforms,
		cl_uint         *num_platforms);

CL_API_ENTRY cl_int CL_API_CALL clGetPlatformInfo(
		cl_platform_id   platform, 
		cl_platform_info param_name,
		size_t           param_value_size, 
		void *           param_value,
		size_t *         param_value_size_ret);


CL_API_ENTRY void* CL_API_CALL clGetExtensionFunctionAddress(const char *funcName);

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDs(
		cl_platform_id platform,
		cl_device_type device_type,
		cl_uint num_entries,
		cl_device_id *devices,
		cl_uint *num_devices);


CL_API_ENTRY cl_int CL_API_CALL clGetDeviceInfo(
		cl_device_id device,
		cl_device_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret);

CL_API_ENTRY cl_int CL_API_CALL clGetPlatformIDs(
		cl_uint          num_entries,
		cl_platform_id * platforms,
		cl_uint *        num_platforms);

CL_API_ENTRY cl_int CL_API_CALL clReleaseMemObject(cl_mem memobj);

}

struct _cl_platform_id{
	struct _cl_icd_dispatch *dispatch;
};
struct _cl_device_id{
	struct _cl_icd_dispatch *dispatch;
	cl_uint id;
};
struct _cl_context{
	struct _cl_icd_dispatch *dispatch;
	//std::list<cl_mem> memList;
	//std::list<cl_program> programList;
};
struct _cl_program {
	struct _cl_icd_dispatch *dispatch;
	cl_context context;
	unsigned int id;
  std::string code;
};
struct _cl_kernel {
  struct ConstDataInfo{
    unsigned char arg_idx;
    size_t size;
  };
	struct _cl_icd_dispatch *dispatch;
	cl_program program;
	unsigned int id;
  std::vector<char> is_pointer;
  std::vector<std::pair<unsigned char, unsigned int> > changed_mem_id;
  std::vector<ConstDataInfo > const_data_info;
  std::vector<char> const_data;
  std::string kernel_name;

  //unsigned char num_of_changed_mem_id;
};
struct _cl_command_queue {
	struct _cl_icd_dispatch *dispatch;
};
struct _cl_event {
	struct _cl_icd_dispatch *dispatch;

};
struct _cl_mem {
	struct _cl_icd_dispatch *dispatch;
	cl_context context;
  cl_mem_object_type type;
	cl_mem_flags flags;
	unsigned int id;
  size_t size;
  void *host_ptr;
	//char * data;
};
#endif  // NEWSCL_CLIENT_CN_API_H_
