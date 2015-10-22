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
using namespace std;
void storeImage(char *file_name, int level);
void recvResult(int level);
void printTime(struct timeval &start);

ElasticClient elasticClient;
uchar **imageo;
cv::Mat* gray = NULL;
typedef int rad[3];
rad *radiusa = NULL;
struct timeval start;

void reSizeImage(char* srcName, char* dstName, float scale){
  IplImage *src = 0;          //來源影像指標
  IplImage *dst = 0;          //目標影像指標
  CvSize dst_cvsize;          //目標影像尺寸

  src = cvLoadImage(srcName);

  dst_cvsize.width = src->width * scale;//目標影像的寬為源影像寬的scale倍
  dst_cvsize.height = src->height * scale;//目標影像的高為源影像高的scale倍

  dst = cvCreateImage( dst_cvsize, src->depth, src->nChannels); //創立目標影像
  cvResize(src, dst, CV_INTER_LINEAR); //縮放來源影像到目標影像
  cvSaveImage(dstName, dst);//儲存影像
}

void displayImage(char * winName, IplImage* image){
  cvNamedWindow(winName, 1);
  cvShowImage(winName, image);
}

void run(char **filename_array, int n){
  double spacestdv = 15;
  double rangestdv = 15;

  int dsize, rsize;
  int mxx = 0 ;
  if(spacestdv > rangestdv)
    mxx = spacestdv;
  else
    mxx = rangestdv;
  int radius = ceil((double)2 * mxx);
  
  //build the bilateral kernel table 
  BilateralFilter *bilateral = new BilateralFilter(spacestdv, rangestdv, &dsize, &rsize);
  gray = new cv::Mat[n];
  uchar **imagei = new uchar*[n];
  imageo = new uchar*[n];
  radiusa = new rad[n];
  for(int i = 0; i < n; i++){
    gray[i] = cv::imread(filename_array[i], CV_LOAD_IMAGE_GRAYSCALE);
    radiusa[i][0] = (int)gray[i].cols * gray[i].elemSize(); //width
    radiusa[i][1] = gray[i].rows; //oheight
    radiusa[i][2] = radius;
    printf("width: %d, height: %d\n", radiusa[i][0], radiusa[i][1]);
    imagei[i] = new uchar[radiusa[i][0] * radiusa[i][1]];
    imageo[i] = new uchar[radiusa[i][0] * radiusa[i][1]];

    for (int j = 0; j < radiusa[i][1]; j++)
        for (int k = 0; k < radiusa[i][0]; k++)
          imagei[i][j * radiusa[i][0] + k] = gray[i].at<uchar>(j, k);
  }

  ElasticIO elasticIO(n);
  for(int i = 0; i < n; i++){
    elasticIO.setInput(i, 0, imagei[i], sizeof(uchar) * radiusa[i][0] * radiusa[i][1]);
    elasticIO.setInput(i, 1, bilateral->clkernelD, sizeof(double) * dsize);
    elasticIO.setInput(i, 2, bilateral->clkernelR, sizeof(double) * rsize );
    elasticIO.setInput(i, 3, radiusa[i], sizeof(int)* 3);
    elasticIO.setOutput(i, 0, imageo[i], sizeof(uchar) * radiusa[i][0] * radiusa[i][1]);
  }
  elasticClient.init("acc.cl", "test", elasticIO);
  elasticClient.setFinishCallback(recvResult);
  gettimeofday(&start, NULL);
  elasticClient.run();


  delete bilateral;
  for(int i = 0; i < n; i++){
    delete[] imagei[i];
    gray[i].release();
  }
  delete[] imagei;
  delete[] gray;
}

void recvResult(int level) {
  printf("level %d is finished\n", level);
  printTime(start);
  char buffer[20];
  sprintf(buffer, "bilateral_%d.jpg", level);
  storeImage(buffer, level);
}

void storeImage(char *file_name, int level){
  uchar *image = imageo[level];
  for (int i = 0; i < radiusa[level][1]; i++){
    for (int j = 0; j < radiusa[level][0]; j++){
      gray[level].at<uchar>(i, j) = image[i * radiusa[level][0] + j];
    }
  }
  printf("file_name : %s\n", file_name);
  cv::imwrite(file_name, gray[level]);

}

void printTime(struct timeval &start){
  struct timeval stop;
  gettimeofday(&stop, NULL);
  float time_use = (stop.tv_sec - start.tv_sec)*1000.0 + (stop.tv_usec - start.tv_usec)/1000.0;
  cout << time_use << "ms" << endl;
}
int main(int argc, char * argv[]){
  const int size = 3;
  float scale = 0.2;

  char *name[] = {"image0.jpg", "image1.jpg", "elsa1.jpg"};
  if(argc > 1){
    name[size - 1] = argv[1];
  }
  if(argc > 2){
    scale = atof(argv[2]);
  }
  float add = (scale + 1) / size;
  for(int i = 0; i < size - 1 ; i++){
    reSizeImage(name[size - 1], name[i], scale);
    scale += add;
  }
/*
  for(int i = size - 2; i >= 0 ; i--){
    reSizeImage(name[size - 1], name[i], scale);
    scale *= 0.5;
  }*/
  run(name, size);
 // char *name[] = {"elsa1.jpg", "lena.bmp"};
//  run(name, 2);
  return 0;
}
