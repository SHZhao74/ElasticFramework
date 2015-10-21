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
void storeImage(char *file_name, int level);
void recvResult(int level);

cl_mem imageo_buf[CN_NUM];
ElasticClient elasticClient;
uchar *imageo1;
uchar *imageo2;
struct timeval start;
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
  imageo1 = new uchar[width * oheight];
  imageo2 = new uchar[width * oheight];

  //grayscale


  for (int i = 0; i < oheight; i++)
    for (int j = 0; j < width; j++)
      imagei[i*width + j] = gray.at<uchar>(i, j);
  
  ElasticIO elasticIO(3);
  elasticIO.setInput(0, 0, imagei, sizeof(uchar) * width * oheight);
  elasticIO.setInput(0, 1, bilateral->clkernelD, sizeof(double) * dsize);
  elasticIO.setInput(0, 2, bilateral->clkernelR, sizeof(double) * rsize );
  elasticIO.setInput(0, 3, radiusa, sizeof(int)* 3);
  elasticIO.setOutput(0, 0, imageo1, sizeof(uchar) * width * oheight);
  
  elasticIO.setInput(1, 0, imagei, sizeof(uchar) * width * oheight);
  elasticIO.setInput(1, 1, bilateral->clkernelD, sizeof(double) * dsize);
  elasticIO.setInput(1, 2, bilateral->clkernelR, sizeof(double) * rsize );
  elasticIO.setInput(1, 3, radiusa, sizeof(int)* 3);
  elasticIO.setOutput(1, 0, imageo2, sizeof(uchar) * width * oheight);

  elasticIO.setInput(2, 0, imagei, sizeof(uchar) * width * oheight);
  elasticIO.setInput(2, 1, bilateral->clkernelD, sizeof(double) * dsize);
  elasticIO.setInput(2, 2, bilateral->clkernelR, sizeof(double) * rsize );
  elasticIO.setInput(2, 3, radiusa, sizeof(int)* 3);
  elasticIO.setOutput(2, 0, imageo1, sizeof(uchar) * width * oheight);
 
  elasticClient.init("acc.cl", "test", elasticIO);
  elasticClient.setFinishCallback(recvResult);
  
  gettimeofday(&start, NULL);
  elasticClient.run();
  
  delete bilateral;
  delete[] imagei;
  gray.release();
}
void printTime(struct timeval &start){
  struct timeval stop;
  gettimeofday(&stop, NULL);
  float time_use = (stop.tv_sec - start.tv_sec)*1000.0 + (stop.tv_usec - start.tv_usec)/1000.0;
  cout << time_use << "ms" << endl;
  
  FILE* fp = fopen("taskTime.txt","a");
  fprintf(fp,"%4f\n", time_use);
  fclose(fp);
}

void recvResult(int level){
  printf("level %d is finish\n", level);
  //char buffer[20];
  //sprintf(buffer, "bilateral_%d.jpg", level);
  //storeImage(buffer, level);

  //FILE* fp = fopen("taskTime.txt","a");
  //fprintf(fp,"%4f\n", (double)(clock()-start)/CLOCKS_PER_SEC);
  //fclose(fp);
  printTime (start);
}

void storeImage(char *file_name, int level){
  char filename[] = "lena.bmp"; // file name
  cv::Mat gray = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

  int z = gray.cols;
  int oheight = gray.rows;
  int width = (int)gray.cols * gray.elemSize();
  uchar *imageo = NULL;
  if(level == 0)imageo = imageo1;
  else if(level == 1)imageo = imageo2;

  for (int i = 0; i < oheight; i++){
    for (int j = 0; j < width; j++){
      gray.at<uchar>(i, j) = imageo[i * width + j];
    }
  }
  printf("file_name : %s\n", file_name);
  cv::imwrite(file_name, gray);
  gray.release();

}


int main(int argc, char * argv[])
{
  FILE* fp = fopen("taskTime.txt","a");
  //struct timeval start;
  int arg = 20;
  if(argc == 2) {
    arg = atoi(argv[1]);
  }

  run(15, 0);

  fprintf(fp,"\n");
  fclose(fp);
  return 0;
}
