#include "CL/cl.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(){
	cl_int errCode;
	cl_uint numOfPlatforms, numOfDevices;
	cl_platform_id *platforms;
	cl_device_id *devices;
	cl_context context;
	cl_program program;
	cl_kernel kernel;
	cl_command_queue queue;
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
	code_str[0] = new char[128];
	sprintf(code_str[0], "__kernel void filter(__global char *tbreak, __global int *times){ for(int i = 0 ; i < 1000000; ++i) if(*tbreak == 1){*times = i;break;} *times = 100000;}");
	//sprintf(code_str[0], "__kernel void filter(){}");
	size_t len = strlen(code_str[0]) + 1;
	program = clCreateProgramWithSource(context, 1, (const char **)code_str, &len, &errCode);
	errCode = clBuildProgram(program, 1, devices, "-cl-opt-disable", NULL, NULL);
	if(errCode != CL_SUCCESS){
		printf("build failed\n");
	}

	kernel = clCreateKernel(program, "filter", &errCode);

	if(errCode != CL_SUCCESS){
		printf("create kernel failed\n");
	}

	queue = clCreateCommandQueue(context, devices[0], 0, &errCode);
	if(errCode != CL_SUCCESS){
		printf("create queue failed\n");
	}

	cl_mem b_break, b_times;
	cl_int err_code;
	char d_break = 0;
	int d_times = 0;
	b_break = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR, sizeof(char), &d_break, &err_code);
	if(err_code != CL_SUCCESS){
		printf("line:%d, err_code:%d\n", __LINE__, err_code);
	}
	errCode = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&(b_break));
	if(errCode != CL_SUCCESS){
		printf("set kernel arg failed\n");
	}

	b_times = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(int), &d_times, &err_code);
	if(err_code != CL_SUCCESS){
		printf("line:%d, err_code:%d\n", __LINE__, err_code);
	}
	errCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&(b_times));
	if(errCode != CL_SUCCESS){
		printf("set kernel arg failed\n");
	}

	// run the kernel

	//sleep(1);

	void* pointer = clEnqueueMapBuffer(queue, b_break, CL_TRUE, CL_MAP_WRITE, 0, sizeof(char), 0, NULL, NULL, &err_code);
	if(err_code != CL_SUCCESS){
		printf("line:%d, err_code:%d\n", __LINE__, err_code);
	}
	
	(*(char*)(pointer)) = 1;
	clEnqueueUnmapMemObject(queue, b_break, pointer, 0, NULL, NULL);
	errCode = clEnqueueTask(queue, kernel, 0, NULL, NULL);
	if(errCode != CL_SUCCESS){
		printf("enqueue task failed\n");
	}
	clFlush(queue);
	 // work with 'original_output'
	clFinish(queue);

	//sleep(1);
	errCode = clEnqueueReadBuffer(queue, b_times, true, 0, sizeof(int), &d_times, 0, NULL, NULL);
	fprintf(stderr, "data[0]: %d\n", d_times);
	//printf("data[0]: %d\n", original_output[1]);
	if(errCode != CL_SUCCESS){
		printf("read buffer enqueue task failed\n");
	}

	//clReleaseMemObject(b_times);
	//clReleaseMemObject(b_break);
	//delete [] devices;
	//free(platforms);
}
