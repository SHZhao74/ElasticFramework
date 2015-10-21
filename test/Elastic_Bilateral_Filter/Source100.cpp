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
#define CN_NUM 1
//#define SELECT_CN 1
using namespace std;
void displayImage(char * winName, IplImage* image){
  cvNamedWindow(winName, 1);
  cvShowImage(winName, image);
}

int main(int argc, char * argv[])
{
  //clock_t begin, end;
  struct timeval start, stop;
  double spacestdv = 15;
  double rangestdv = 30;

  int SELECT_CN = 0;
  if(argc == 2)
    rangestdv = atoi(argv[1]);
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
  gettimeofday(&start, NULL);
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
  
  int num = CN_NUM;
  switch(num){
  case 3:
    WUPK[0] = width * oheight/7;
    WUPK[1] = width * oheight*3/7;
    WUPK[2] = width * oheight*3/7;
    offset[0] = 0;
    offset[1] = WUPK[0];
    offset[2] = offset[1]+WUPK[1];
    break;
  case 2:
    WUPK[0] = width * oheight/3;
    WUPK[1] = width * oheight*2/3;
    offset[0] = 0;
    offset[1] = WUPK[0];
    break;
  case 1:
    WUPK[0] = width * oheight;
    offset[0] = 0;
    break;
  }

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
   // begin = clock();
  
    pro[i] = clCreateProgramWithSource(i + SELECT_CN, 1, (const char**)&PB, &PS, &err);
    if (err < 0) {
      perror("Couldn't create the program");
      exit(1);
    }
   // end = clock();
   // printf("CN %d CreateProgram: %lfms\n", i, (double)(end - begin)*1000 / CLOCKS_PER_SEC);
  }

  free(PB);
  for(int i = 0; i < CN_NUM; i++){
    err = clBuildProgram(i + SELECT_CN, pro[i], 0, NULL, NULL, NULL, NULL);
    kernel1[i] = clCreateKernel(i + SELECT_CN, pro[i], "test", &err);
  }
  if (err < 0) {
    perror("Couldn't create the kernel");
    exit(1);
  }

  for(int i = 0; i < CN_NUM; i++){
   // begin = clock();
    imagei_buf[i] = clCreateBuffer(i + SELECT_CN, CL_MEM_READ_ONLY, sizeof(uchar)*width * oheight, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object first");
      printf("%d", err);
      exit(1);
    }
    clkernelD_buf[i] = clCreateBuffer(i + SELECT_CN, CL_MEM_READ_ONLY, sizeof(double)*dsize, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object A");
      exit(1);
    }
    clkernelR_buf[i] = clCreateBuffer(i + SELECT_CN, CL_MEM_READ_ONLY, sizeof(double)*rsize, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object B");
      exit(1);
    }
    clradius_buf[i] = clCreateBuffer(i + SELECT_CN, CL_MEM_READ_ONLY, sizeof(int)* 3, NULL, &err);
    if (err < 0) {
      perror("Couldn't create a buffer object C");
      exit(1);
    }
    imageo_buf[i] = clCreateBuffer(i + SELECT_CN, CL_MEM_WRITE_ONLY, sizeof(uchar)*(width * oheight), NULL, &err);
    if (err < 0) {
      perror("Couldn't create a res_buffer");
      exit(1);
    }
    clEnqueueWriteBuffer(i+ SELECT_CN, imagei_buf[i], CL_TRUE, offset[i], sizeof(uchar)*WUPK[i], imagei, 0, NULL, 0);
    clEnqueueWriteBuffer(i+ SELECT_CN, clkernelD_buf[i], CL_TRUE, 0, sizeof(double)*dsize, bilateral->clkernelD, 0, NULL, 0);
    clEnqueueWriteBuffer(i+ SELECT_CN, clkernelR_buf[i], CL_TRUE, 0, sizeof(double)*rsize, bilateral->clkernelR, 0, NULL, 0);
    clEnqueueWriteBuffer(i+ SELECT_CN, clradius_buf[i], CL_TRUE, 0, sizeof(int)* 3, radiusa, 0, NULL, 0);

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
   // end = clock();
   // printf("CN %d CreateBuffer: %lfms\n", i, (double)(end - begin)*1000 / CLOCKS_PER_SEC);

  }
  cl_event evt;
  
  //size_t offset = width * oheight/2;
  

  for(int i = 0; i < CN_NUM; i++){
 //   begin = clock();
    err = clSetKernelArg(kernel1[i], 5, sizeof(unsigned int), &mxx);
  
    err = clEnqueueNDRangeKernel(i+ SELECT_CN, kernel1[i], 1, NULL, &(WUPK[i]), 0, 0, NULL, NULL);

    if (err < 0) {
      perror("Couldn't enqueue the kernel execution command");
      exit(1);
    }
  //  end = clock();
 //   printf("CN %d EnqueueNDRangeKernel: %lfs\n", i, (double)(end - begin) / CLOCKS_PER_SEC);

  }
  /*char aaaa[2];
  std::cin >> aaaa;
  */
  for(int j = 0; j < CN_NUM; j++){
 // int j = 2;
   // gettimeofday(&start, NULL);
   // begin = clock();
    err = clEnqueueReadBuffer(j, imageo_buf[j], CL_TRUE, 0, sizeof(uchar)*(WUPK[j]), imagei, 0, NULL, NULL);
   // end = clock();
    //gettimeofday(&stop, NULL);
   // printf("CN %d execution and ReadBuffer: %fs\n", j, (stop.tv_usec - start.tv_usec)/1000000+ );

  }
  if (err < 0) {
    perror("Couldn't enqueue the read buffer command");
    std::cout<<"err code" << err << std::endl;
    system("pause");
    exit(1);
  }
  //cout << (clock()-start_time)/CLOCKS_PER_SEC << "s" << endl;

  gettimeofday(&stop, NULL);
  float time_use = (stop.tv_sec - start.tv_sec)*1000.0 + (stop.tv_usec - start.tv_usec)/1000.0;
  cout << time_use << "ms" << endl;
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
