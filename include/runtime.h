// Copyright [2013] (JoenChen: joen@joen.cc)
#ifndef NEWSCL_CLIENT_RUNTIME_H_
#define NEWSCL_CLIENT_RUNTIME_H_

#include "IO.h"
#include "config.h"
#include <string>
namespace newscl {
	_cl_icd_dispatch* GetDispatch();
	_cl_platform_id* GetBestPlatforms(const std::string appName, const float deadline, const int levelCnt); 
	cl_platform_id GetPlatform();
	cl_context GetContext();
	cl_device_id GetDevice();
	cl_command_queue GetCommandQueue();
	cl_device_id GetPhysicalDevicesInfo(unsigned int dev_type);
	SocketIOClient* GetDispatcherConnection(); 
	SocketIOClient* GetConnection(const std::string &ip = ""); 
	SocketIOClient* GetFinishConnection(const std::string &ip = ""); 
  double& GetWeight(double const &weight = 0.f);


  void* clGetExtensionFunctionAddressForPlatform(cl_platform_id  platform,
                                                 const char  *funcname);

  cl_int clGetKernelWorkGroupInfo(cl_kernel kernel,
                                  cl_device_id device,
                                  cl_kernel_work_group_info param_name,
                                  size_t param_value_size,
                                  void *param_value,
                                  size_t *param_value_size_ret);
  cl_int clBullet3ReadResultBack(void* data, cl_uint data_size);
  cl_int clBullet3NPWriteToGPU(char* t_data,
                               char* t_data_size,
                               unsigned long long t_total_data_size,
                               unsigned long long num_of_data,
                               unsigned long long num_of_bodies);
  cl_int clBullet3RBPWriteToGPU(char* t_data,
                               char* t_data_size,
                               unsigned long long t_total_data_size,
                               unsigned long long num_of_data);
  cl_int clBullet3BPWriteToGPU(char* t_data,
                               char* t_data_size,
                               unsigned long long t_total_data_size,
                               unsigned long long num_of_data);
  cl_int clReleaseMemObject(cl_mem memobj);
  cl_int clReleaseProgram(cl_program program);
  cl_int clReleaseKernel(cl_kernel kernel);
  cl_int clReleaseEvent(cl_event event);
  cl_int clReleaseCommandQueue(cl_command_queue command_queue);
  cl_int clReleaseDevice(cl_device_id device);
  cl_int clReleaseContext(cl_context context);
  cl_int clGetProgramBuildInfo(cl_program program,
                                 cl_device_id device,
                                 cl_program_build_info param_name,
                                 size_t param_value_size,
                                 void *param_value,
                                 size_t *param_value_size_ret);
  cl_int clEnqueueCopyBuffer(cl_command_queue command_queue,
                             cl_mem src_buffer,
                             cl_mem dst_buffer,
                             size_t src_offset,
                             size_t dst_offset,
                             size_t cb,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list,
                             cl_event *event);
  
  cl_int clGetMemObjectInfo(cl_mem memobj,
                            cl_mem_info param_name,
                            size_t param_value_size,
                            void *param_value,
                            size_t *param_value_size_ret);
	cl_int clFinish(cl_command_queue);
	cl_int clGetContextInfo(cl_context         /* context */, 
			cl_context_info    /* param_name */, 
			size_t             /* param_value_size */, 
			void *             /* param_value */, 
			size_t *           /* param_value_size_ret */) ;

	cl_context clCreateContextFromType(const cl_context_properties * /* properties */,
				cl_device_type          /* device_type */,
				void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *), 
				void *                  /* user_data */,
				cl_int *                /* errcode_ret */) ;


	cl_int clSetKernelArg (	cl_kernel kernel,
			cl_uint arg_index,
			size_t arg_size,
			const void *arg_value);

	cl_int clEnqueueWriteBuffer (	cl_command_queue command_queue,
			cl_mem buffer,
			cl_bool blocking_write,
			size_t offset,
			size_t cb,
			const void *ptr,
			cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list,
			cl_event *event);

	cl_int clEnqueueReadBuffer (	cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_read,
		size_t offset,
		size_t cb,
		void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event);
	
	cl_mem clCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size,
               void *       host_ptr,
               cl_int *     errcode_ret);

	cl_int clEnqueueTask(cl_command_queue command_queue,
                        cl_kernel kernel,
			cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list,
			cl_event *event);

  cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue,
                                cl_kernel kernel,
                                cl_uint work_dim,                                     
                                const size_t *global_work_offset,                     
                                const size_t *global_work_size,                       
                                const size_t *local_work_size,                        
                                cl_uint num_events_in_wait_list,                      
                                const cl_event *event_wait_list,                      
                                cl_event *event);

	cl_kernel clCreateKernel (cl_program  program,
			const char *kernel_name,
			cl_int *errcode_ret);

	cl_int clBuildProgram (	cl_program program,
			cl_uint num_devices,
			const cl_device_id *device_list,
			const char *options,
			void (*pfn_notify)(cl_program, void *user_data),
			void *user_data);

	cl_command_queue clCreateCommandQueue(	cl_context context,
			cl_device_id device,
			cl_command_queue_properties properties,
			cl_int *errcode_ret);

	cl_program clCreateProgramWithSource(cl_context       context,
			cl_uint           count ,
			const char **     strings ,
			const size_t *    lengths ,
			cl_int *          errcode_ret);

	cl_int clGetDeviceIDs(
			cl_platform_id platform,
			cl_device_type device_type,
			cl_uint num_entries,
			cl_device_id *devices,
			cl_uint *num_devices);

	cl_int clGetDeviceInfo(
			cl_device_id device,
			cl_device_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t *param_value_size_ret);

	cl_int clGetPlatformInfo(
			cl_platform_id   platform, 
			cl_platform_info param_name,
			size_t           param_value_size, 
			void *           param_value,
			size_t *         param_value_size_ret);

	void* clGetExtensionFunctionAddress(const char *funcName);

	cl_int clGetPlatformIDs(
			cl_uint          num_entries,
			cl_platform_id * platforms,
			cl_uint *        num_platforms);

	cl_context clCreateContext(const cl_context_properties * /* properties */,
			cl_uint                 /* num_devices */,
			const cl_device_id *    /* devices */,
			void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
			void *                  /* user_data */,
			cl_int *                /* errcode_ret */);
}  // namespace newscl
#endif  // NEWSCL_CLIENT_RUNTIME_H_
