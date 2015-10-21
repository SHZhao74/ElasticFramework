#include "CL/cl.h"
#include <assert.h>
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
	cl_context_properties props[3];
	props[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;  // indicates that next element is platform
	props[1] = (cl_context_properties)platforms[0];  // platform is of type cl_platform_id
	props[2] = (cl_context_properties)0;   // last element must be 0

	context = clCreateContextFromType(props, CL_DEVICE_TYPE_ALL, NULL, NULL, &errCode);
	//context = clCreateContext(NULL, 1, devices, NULL, NULL, &errCode);
	if(context == NULL){
		printf("get device error\n");
  }
  cl_int (*fp_clBullet3ReadResultBack)(void* data, cl_uint data_size);      
  fp_clBullet3ReadResultBack = (cl_int(*)(void* data, cl_uint)) clGetExtensionFunctionAddressForPlatform(
      platforms[0],                                                                   
      "clBullet3ReadResultBack");                                                 
  char data[2000000];
  fp_clBullet3ReadResultBack(data, 17440);  
  FILE *fp = fopen("data", "wb");
  assert(fp!=NULL);
  fwrite(data, 1, 17440,fp);
  fclose(fp);
  
	delete [] devices;
	free(platforms);
}
