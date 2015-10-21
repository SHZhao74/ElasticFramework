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
#define CN_NUM 2

void displayImage(char * winName, IplImage* image){
  cvNamedWindow(winName, 1);
  cvShowImage(winName, image);
}

int main(int argc, char * argv[])
{
  int plat = 0;
  if(argc == 2)
    plat = atoi(argv[1]);
  //space & range weight
  double spacestdv = 15;
  double rangestdv = 20;
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

  //char filename[] = "elsa1.jpg"; // file name
  char filename[] = "lena.bmp"; // file name
  printf("%s \n", filename);


  //cv::Mat gray = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
  cv::Mat gray = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
  //cv::imshow("original1", gray);


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






  //RGB
  /*
     for (int i = 0; i < oheight; i++){
     for (int j = 0; j < width; j += 3){
     imagei[i*width + j] = gray.at<uchar>(i, j);
     imagei[i*width + j + 1] = gray.at<uchar>(i, j + 1);
     imagei[i*width + j + 2] = gray.at<uchar>(i, j + 2);
     }

     }

     for (int i = 0; i < oheight; i++){
     for (int j = 0; j < width; j += 3){
     gray.at<uchar>(i, j) = 0;
     gray.at<uchar>(i, j + 1) = 0;
     gray.at<uchar>(i, j + 2) = 0;
     }

     }
     */

  // imagei[],imageo[] , bilateral->clkernelD[] (dsize), bilateral->clkernelR[] (rsize)
  // ----------------------------------- OpenCL -----------------------------------------------

  cl_platform_id Pid[2];
  cl_int i, err, status;


  cl_program pro[CN_NUM];
  FILE* PH;
  char* PB, *PL;
  size_t PS, LS;
  cl_kernel kernel1[CN_NUM];
  size_t WUPK[CN_NUM];
  size_t offset[CN_NUM]; 
  cl_ulong size, startTime, endTime;


  cl_mem imagei_buf[CN_NUM], imageo_buf[CN_NUM], clkernelD_buf[CN_NUM], clkernelR_buf[CN_NUM], clradius_buf[CN_NUM];
  GetPlatform();
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

  for(int i = 0; i < CN_NUM; i++){
    pro[i] = clCreateProgramWithSource(i, 1, (const char**)&PB, &PS, &err);
    if (err < 0) {
      perror("Couldn't create the program");
      exit(1);
    }
  }

  free(PB);
  for(int i = 0; i < CN_NUM; i++){
    err = clBuildProgram(i, pro[i], 0, NULL, NULL, NULL, NULL);
    kernel1[i] = clCreateKernel(i, pro[i], "test", &err);
  }
  if (err < 0) {
    perror("Couldn't create the kernel");
    exit(1);
  }

  for(int i = 0; i < CN_NUM; i++){
    imagei_buf[i] = clCreateBuffer(i, CL_MEM_READ_ONLY, sizeof(uchar)*width * oheight, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object first");
      printf("%d", err);
      exit(1);
    }
    clkernelD_buf[i] = clCreateBuffer(i, CL_MEM_READ_ONLY, sizeof(double)*dsize, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object A");
      exit(1);
    }
    clkernelR_buf[i] = clCreateBuffer(i, CL_MEM_READ_ONLY, sizeof(double)*rsize, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object B");
      exit(1);
    }
    clradius_buf[i] = clCreateBuffer(i, CL_MEM_READ_ONLY, sizeof(int)* 3, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object C");
      exit(1);
    }
    imageo_buf[i] = clCreateBuffer(i, CL_MEM_WRITE_ONLY, sizeof(uchar)*(width * oheight), NULL, &err);
    if (err < 0) {
      perror("Couldn't create a res_buffer");
      exit(1);
    }
  clEnqueueWriteBuffer(i, imagei_buf[i], CL_TRUE, 0, sizeof(uchar)*width * oheight, imagei, 0, NULL, 0);
  clEnqueueWriteBuffer(i, clkernelD_buf[i], CL_TRUE, 0, sizeof(double)*dsize, bilateral->clkernelD, 0, NULL, 0);
  clEnqueueWriteBuffer(i, clkernelR_buf[i], CL_TRUE, 0, sizeof(double)*rsize, bilateral->clkernelR, 0, NULL, 0);
  clEnqueueWriteBuffer(i, clradius_buf[i], CL_TRUE, 0, sizeof(int)* 3, radiusa, 0, NULL, 0);

  err = clSetKernelArg(kernel1[i], 0, sizeof(cl_mem), &(imagei_buf[i]));
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1[i], 1, sizeof(cl_mem), &clkernelD_buf[i]);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1[i], 2, sizeof(cl_mem), &clkernelR_buf[i]);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1[i], 3, sizeof(cl_mem), &clradius_buf[i]);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
  err = clSetKernelArg(kernel1[i], 4, sizeof(cl_mem), &imageo_buf[i]);
  if (err < 0) {
    perror("Couldn't set the kernel argument");
    exit(1);
  }
}
  cl_event evt;

  //size_t offset = width * oheight/2;
  WUPK[0] = width * oheight/5;
  WUPK[1] = width * oheight*2/5;
  WUPK[2] = width * oheight*2/5;
  offset[0] = 0;
  offset[1] = WUPK[0];
  offset[2] = offset[1]+WUPK[1];

  for(int i = 0; i < CN_NUM; i++){
    err = clSetKernelArg(kernel1[i], 5, sizeof(unsigned int), &offset[i]);
  }
    err = clEnqueueNDRangeKernel(1, kernel1[1], 1, NULL, &(WUPK[1]), 0, 0, NULL, NULL);

    if (err < 0) {
      perror("Couldn't enqueue the kernel execution command");
      exit(1);
    }
//  }
//  for(int i = 0; i < CN_NUM; i++){
    err = clEnqueueReadBuffer(1, imageo_buf[1], CL_TRUE, 0, sizeof(uchar)*(WUPK[1]), imagei, 0, NULL, &evt);
//  }
  //err = clEnqueueReadBuffer(Cqueue, imageo_buf, CL_FALSE, 0, sizeof(uchar)*(width * oheight), imagei, 0, NULL, &evt);
  if (err < 0) {
    perror("Couldn't enqueue the read buffer command");
    std::cout<<"err code" << err << std::endl;
    system("pause");
    exit(1);
  }



  // Get kernel profiling info


  //grayscale

  for (int i = 0; i < oheight; i++){
    for (int j = 0; j < width; j++){
      gray.at<uchar>(i, j) = imagei[i * width + j];
    }
  }

  //RGB
  /*
     for (int i = 0; i < oheight; i++){
     for (int j = 0; j < width; j += 3){
     gray.at<uchar>(i, j) = imageo[i * width + j];
     gray.at<uchar>(i, j + 1) = imageo[i * width + j + 1];
     gray.at<uchar>(i, j + 2) = imageo[i * width + j + 2];
     }
     }
     */
  /*	
        clReleaseMemObject(imagei_buf);
        clReleaseMemObject(clkernelD_buf);
        clReleaseMemObject(clkernelR_buf);
        clReleaseMemObject(clradius_buf);
        clReleaseMemObject(imageo_buf);
        clReleaseKernel(kernel1);
        clReleaseCommandQueue(Cqueue);
        clReleaseProgram(pro);
        clReleaseContext(Ctext);
        */	

  double GpuTime =  1e-9 *endTime - 1e-9 *startTime ;

  FILE *fp;
  fp = fopen("rstGPU.csv", "a");

  fputc('\n', fp);
  fprintf(fp, "%dx%d,GPU,%f", oheight, width,  GpuTime);


  delete[] imagei;

  fclose(fp);

  //cv::imshow("afterCPU", gray);
  cv::imwrite("bilateral.jpg", gray);
  //cvWaitKey(0);
  gray.release();

  return 0;
}
