#include "CL/cl.h"
#include "CL/common.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
double aclGetExecTime2(cl_event event){
	cl_ulong start, end;
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong), &end, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong), &start, NULL);
	return (end - start) * 1.0e-9f;
}
int main(){
	cl_int errCode;
	cl_uint numOfPlatforms, numOfDevices;
	cl_platform_id *platforms;
	cl_device_id *devices;
	cl_context context;
	cl_program program;
	cl_kernel kernel;
	cl_command_queue queue,queue2;
	cl_mem mem;

	char str[1024] = "";
	errCode = clGetPlatformIDs(0, NULL, &numOfPlatforms);
	platforms = new cl_platform_id[numOfPlatforms];

	errCode = clGetPlatformIDs(numOfPlatforms, platforms, NULL);
	printf("numOfPlatforms: %d\n", numOfPlatforms);
	for(cl_uint i = 0, numOfDevices; i < numOfPlatforms; ++i){
		cl_device_id *devices = NULL;

		errCode = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, str, NULL);
		printf("platform %d name: %s, errCode: %d\n", i, str, errCode);
		printf("platform name: %s\n", str);

		errCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numOfDevices);
		devices = new cl_device_id[numOfDevices];
		errCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numOfDevices, devices, NULL);

		for(cl_uint j = 0; j < numOfDevices; ++j){
			errCode = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 1024, str, NULL);
			printf("device %d name: %s\n", j, str);
		}
		delete [] devices;
	}
	devices = new cl_device_id[1];
	errCode = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 1, devices, &numOfDevices);
	printf("numOfDevices: %d\n", numOfDevices);

	context = clCreateContext(NULL, 1, devices, NULL, NULL, &errCode);
	if(context == NULL){
		printf("get device error\n");
	}


	char** code_str;
	code_str = new char*[1];
	code_str[0] = new char[1024];
	bzero(code_str[0], 1024);
	FILE *fp = fopen("./cltest.c", "r");
	if(fp == NULL){
		fprintf(stderr, "failed to open\n");
	}
	fread(reinterpret_cast<void*>(code_str[0]), sizeof(char), 1024, fp);
	fprintf(stderr, "code:%s\n", code_str[0]);
	//sprintf(code_str[0], "__kernel void filter(){}");
	size_t len = strlen(code_str[0]) + 1;
	program = clCreateProgramWithSource(context, 1, (const char **)code_str, &len, &errCode);
	_ED(errCode = clBuildProgram(program, 1, devices, "-cl-opt-disable", NULL, NULL));

	kernel = clCreateKernel(program, "filter", &errCode);
	getErrorMsg(errCode);

	queue = clCreateCommandQueue(context, devices[0], 0, &errCode);
	queue2 = clCreateCommandQueue(context, devices[0], 0, &errCode);
	getErrorMsg(errCode);

	cl_event event;
	cl_mem b_break, b_times;
	cl_int err_code;
	char d_break = 0;
	int d_times = 0;
	void *pointer;
	//b_break = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(char), &d_break, &errCode);
	b_break = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_PERSISTENT_MEM_AMD, sizeof(char), NULL, &errCode);
	getErrorMsg(errCode);

	pointer = clEnqueueMapBuffer(queue2, b_break, CL_TRUE, CL_MAP_WRITE, 0, sizeof(char), 0, NULL, NULL, &errCode);
	getErrorMsg(errCode);
	
	bzero(pointer, 1);
	_ED(clEnqueueUnmapMemObject(queue2, b_break, pointer, 0, NULL, NULL));
	clFinish(queue2);

	_ED(errCode = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&(b_break)));

	b_times = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(int), &d_times, &errCode);
	getErrorMsg(errCode);

	_ED(errCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&(b_times)));

	pointer = clEnqueueMapBuffer(queue, b_break, CL_TRUE, CL_MAP_WRITE, 0, sizeof(char), 0, NULL, NULL, &errCode);
	getErrorMsg(errCode);

	_ED(errCode = clEnqueueTask(queue, kernel, 0, NULL, &event));
	_ED(clFlush(queue));
	// run the kernel
	//sleep(1);
	//usleep(500);

	
	(*(char*)(pointer)) = 1;
	cl_event event2;
	_ED(clEnqueueUnmapMemObject(queue, b_break, pointer, 0, NULL, &event2));
	usleep(500);

	 // work with 'original_output'
	_ED(clFinish(queue));
	//_ED(clFinish(queue2));
	if(queue == queue2){
		fprintf(stderr, "hi!!!!!!!!!!\n");
	}

	_ED(clEnqueueReadBuffer(queue, b_times, true, 0, sizeof(int), &d_times, 0, NULL, NULL));

	fprintf(stderr, "data[0]: %d\n", d_times);
	fprintf(stderr, "exectime:%f\n", aclGetExecTime(event));
	fprintf(stderr, "exectime:%f\n", aclGetExecTime2(event2));

	clReleaseMemObject(b_times);
	clReleaseMemObject(b_break);
	//delete [] devices;
	//free(platforms);
}
