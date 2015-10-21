#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "elastic.h"
#include "CL/cl.h"

cl_program load_program(int devices, const char* filename)
{
  std::ifstream in(filename, std::ios_base::binary);
  if(!in.good()) {
    return 0;
  }

  // get file length
  in.seekg(0, std::ios_base::end);
  size_t length = in.tellg();
  in.seekg(0, std::ios_base::beg);

  // read program source
  std::vector<char> data(length + 1);
  in.read(&data[0], length);
  data[length] = 0;
  // create and build program 
  const char* source = &data[0];
  cl_program program = clCreateProgramWithSource(devices,  1, &source, 0, 0);
  if(program == 0) {
    return 0;
  }

  if(clBuildProgram(devices, program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
    return 0;
  }

  return program;
}

int main(int argc,char* argv[]){
  int devices = 0;
  if(argc == 2){
    devices = atoi(argv[1]);
  }
  GetPlatform();
  const int DATA_SIZE = 100;
  std::vector<float> a(DATA_SIZE), b(DATA_SIZE), res(DATA_SIZE);
  for(int i = 0; i < DATA_SIZE; i++) {
    a[i] = std::rand();
    b[i] = std::rand();
  }
  cl_mem cl_a = clCreateBuffer(devices, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &a[0], NULL);
  cl_mem cl_b = clCreateBuffer(devices, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &b[0], NULL);
  cl_mem cl_res = clCreateBuffer(devices, CL_MEM_WRITE_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
  cl_program program = load_program(devices, "shader.cl");
  if(program == 0) {
    std::cerr << "Can't load or build program\n";
    return 0;
  }

  cl_kernel adder = clCreateKernel(devices, program, "adder", 0);
  if(adder == 0) {
    std::cerr << "Can't load kernel\n";
    return 0;
  }
  clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
  clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_b);
  clSetKernelArg(adder, 2, sizeof(cl_mem), &cl_res);

  size_t work_size = DATA_SIZE;
  cl_int err = clEnqueueNDRangeKernel(devices, adder, 1, 0, &work_size, 0, 0, 0, 0);

  if(err == CL_SUCCESS) {
    err = clEnqueueReadBuffer(devices, cl_res, CL_TRUE, 0, sizeof(float) * DATA_SIZE, &res[0], 0, 0, 0);
    bool correct = true;
    for(int i = 0; i < DATA_SIZE; i++) {
      if(a[i] + b[i] != res[i]) {
        correct = false;
        break;
      }
    }
    if(correct){
      std::cout << "correct\n";
    } else{
      std::cout << "incorrect\n";
    }
  }

  return 0;
}
