#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include "elastic.h"
#include "CL/cl.h"
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "BilateralFilter.h"
#include <iostream>
#include <sys/time.h>
#include <time.h>
#define CN_NUM 2
//#define SELECT_CN 1
using namespace std;

cl_mem imageo_buf[CN_NUM];

void displayImage(char * winName, IplImage* image){
  cvNamedWindow(winName, 1);
  cvShowImage(winName, image);
}
void run(int arg, int cn_id){
  double spacestdv = 15;
  double rangestdv = arg;

  int dsize, rsize;
  int mxx =0 ;
  if(spacestdv > rangestdv)
    mxx = spacestdv;
  else
    mxx = rangestdv;
  int radius = ceil((double)2 * mxx);
  int radiusa[3] = { 0 };

  //build the bilateral kernel table 
  BilateralFilter *bilateral = new BilateralFilter(spacestdv, rangestdv, &dsize, &rsize);

  // char filename[] = "elsa1.jpg"; // file name
  char filename[] = "lena.bmp"; // file name
  //  char filename[] = "photo.jpg";
  printf("%s \n", filename);

  cv::Mat gray = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

  int z = gray.cols;
  int oheight = gray.rows;
  printf("weight = %d  , height = %d \n", z, oheight);


  //channel * rows;
  int width = (int)gray.cols*gray.elemSize();
  radiusa[0] = width;
  radiusa[1] = oheight;
  radiusa[2] = radius;

  uchar *imagei;
  imagei = new uchar[width * oheight];

  //grayscale


  for (int i = 0; i < oheight; i++)
    for (int j = 0; j < width; j++)
      imagei[i*width + j] = gray.at<uchar>(i, j);


  cl_int i, err, status;


  cl_program pro;
  FILE* PH;
  char* PB, *PL;
  size_t PS, LS;
  size_t WUPK = width * oheight;
  cl_kernel kernel1;
  cl_mem imagei_buf, clkernelD_buf, clkernelR_buf, clradius_buf;
  PH = fopen("acc2.cl", "rb");
  if (PH == NULL) {
    perror("Couldn't find the program file");
    exit(1);
  }
  fseek(PH, 0, SEEK_END);
  PS = ftell(PH);
  rewind(PH);
  PB = (char*)malloc(PS + 1);
  PB[PS] = '\0';
  fread(PB, sizeof(char), PS, PH);
  fclose(PH);

  pro = clCreateProgramWithSource(cn_id, 1, (const char**)&PB, &PS, &err);
  if (err < 0) {
    perror("Couldn't create the program");
    exit(1);
  }

  free(PB);
  err = clBuildProgram(cn_id, pro, 0, NULL, NULL, NULL, NULL);
  kernel1 = clCreateKernel(cn_id, pro, "test", &err);
  if (err < 0) {
    perror("Couldn't create the kernel");
    exit(1);
  }

  imagei_buf = clCreateBuffer(cn_id, CL_MEM_READ_ONLY, sizeof(uchar)*width * oheight, NULL, &err);
  if (err < 0) {
    perror("Couldn't create a buffer object first");
    printf("%d", err);
    exit(1);
  }
  clkernelD_buf = clCreateBuffer(cn_id, CL_MEM_READ_ONLY, sizeof(double)*dsize, NULL, &err);
  if (err < 0) {
    perror("Couldn't create a buffer object A");
    exit(1);
  }
  clkernelR_buf = clCreateBuffer(cn_id, CL_MEM_READ_ONLY, sizeof(double)*rsize, NULL, &err);
  if (err < 0) {
    perror("Couldn't create a buffer object B");
    exit(1);
  }
  clradius_buf = clCreateBuffer(cn_id, CL_MEM_READ_ONLY, sizeof(int)* 3, NULL, &err);
  if (err < 0) {
    perror("Couldn't create a buffer object C");
    exit(1);
  }
  imageo_buf[cn_id] = clCreateBuffer(cn_id, CL_MEM_WRITE_ONLY, sizeof(uchar)*(width * oheight), NULL, &err);
  if (err < 0) {
    perror("Couldn't create a res_buffer");
    exit(1);
  }
  clEnqueueWriteBuffer(cn_id, imagei_buf, CL_TRUE, 0, sizeof(uchar)*width * oheight, imagei, 0, NULL, 0);
  clEnqueueWriteBuffer(cn_id, clkernelD_buf, CL_TRUE, 0, sizeof(double)*dsize, bilateral->clkernelD, 0, NULL, 0);
  clEnqueueWriteBuffer(cn_id, clkernelR_buf, CL_TRUE, 0, sizeof(double)*rsize, bilateral->clkernelR, 0, NULL, 0);
  clEnqueueWriteBuffer(cn_id, clradius_buf, CL_TRUE, 0, sizeof(int)* 3, radiusa, 0, NULL, 0);

  err = clSetKernelArg(kernel1, 0, sizeof(cl_mem), &(imagei_buf));
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1, 1, sizeof(cl_mem), &clkernelD_buf);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1, 2, sizeof(cl_mem), &clkernelR_buf);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1, 3, sizeof(cl_mem), &clradius_buf);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1, 4, sizeof(cl_mem), &imageo_buf[cn_id]);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }

  cl_event evt;

  err = clSetKernelArg(kernel1, 5, sizeof(unsigned int), &mxx);
  err = clEnqueueNDRangeKernel(cn_id, kernel1, 1, NULL, &WUPK, 0, 0, NULL, NULL);

  if (err < 0) {
    perror("Couldn't enqueue the kernel execution command");
    exit(1);
  }
  delete bilateral;
  delete[] imagei;
  gray.release();
}
void readResult(char *file_name, int cn_id){
  char filename[] = "lena.bmp"; // file name
  cv::Mat gray = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

  int z = gray.cols;
  int oheight = gray.rows;
  int width = (int)gray.cols * gray.elemSize();

  uchar *imagei = new uchar[z * oheight];

  cl_int err = clEnqueueReadBuffer(cn_id, imageo_buf[cn_id], CL_TRUE, 0, sizeof(uchar)*512*512, imagei, 0, NULL, NULL);
  if (err < 0) {
    perror("Couldn't enqueue the read buffer command");
    std::cout<<"err code" << err << std::endl;
    exit(1);
  }
  for (int i = 0; i < oheight; i++){
    for (int j = 0; j < width; j++){
      gray.at<uchar>(i, j) = imagei[i * width + j];
    }
  }
  cv::imwrite(file_name, gray);
  gray.release();
  delete[] imagei;

}

void printTime(struct timeval &start){
  struct timeval stop;
  gettimeofday(&stop, NULL);
  float time_use = (stop.tv_sec - start.tv_sec)*1000.0 + (stop.tv_usec - start.tv_usec)/1000.0;
  cout << time_use << "ms" << endl;
}

int main(int argc, char * argv[])
{
  struct timeval start;
  gettimeofday(&start, NULL);
  int arg = 20;
  if(argc == 2) {
    arg = atoi(argv[1]);
  }
  GetPlatform();
  gettimeofday(&start, NULL);

  run(15, 1);
  printTime(start);
  run(25, 0);
  printTime(start);
 // run(30, 0);
  readResult("bilateral_1.jpg", 1);
  printTime(start);
  readResult("bilateral_0.jpg", 0);
  printTime(start);

/*
  run (arg, 0);
  printTime(start);
  if(CN_NUM >= 2){
    run (arg - 10, 1);
    printTime(start);
  }
  if(CN_NUM >= 2){
    readResult("bilateral_1.jpg", 1);
    printTime(start);
  }
  readResult("bilateral_0.jpg", 0);
  printTime(start);
*/
  return 0;
}
