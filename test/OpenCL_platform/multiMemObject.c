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
	cl_mem mems[5];

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
	cl_context_properties props[3];
	props[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;  // indicates that next element is platform
	props[1] = (cl_context_properties)platforms[0];  // platform is of type cl_platform_id
	props[2] = (cl_context_properties)0;   // last element must be 0

	context = clCreateContextFromType(props, CL_DEVICE_TYPE_ALL, NULL, NULL, &errCode);
	//context = clCreateContext(NULL, 1, devices, NULL, NULL, &errCode);
	if(context == NULL){
		printf("get device error\n");
	}

	queue = clCreateCommandQueue(context, devices[0], 0, &errCode);
	if(errCode != CL_SUCCESS){
		printf("create queue failed\n");
	}

  for ( int i = 0; i < 5 ; ++i) {
	  char data[1024];
    char data2[1024];
	  mems[i] = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, 1024, data, &errCode);
    fprintf(stderr, "create buffer done\n");
	  if(errCode != CL_SUCCESS){
		  printf("create buffer enqueue task failed\n");
	  }
	  memset(data, 1, 1024);
    clEnqueueReadBuffer(queue, mems[i], false, 0, 99, data2, 0, NULL,NULL);
  }
	
	delete [] devices;
	free(platforms);
}
