#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<CL/cl.h>
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "BilateralFilter.h"
#include <iostream>

void displayImage(char * winName, IplImage* image){
	cvNamedWindow(winName, 1);
	cvShowImage(winName, image);
}

int main(int argc, char * argv[])
{
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

		//char *filename = "elsa1.jpg"; // file name
		char *filename = "lena.bmp"; // file name
		
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
		
		cl_platform_id Pid;
		cl_device_id Did;
		cl_context Ctext;
		cl_command_queue Cqueue;
		cl_int i, err, status;


		cl_program pro;
		FILE* PH;
		char* PB, *PL;
		size_t PS, LS;
		cl_kernel kernel1;
		size_t WUPK;
		cl_ulong size, startTime, endTime;


		cl_mem imagei_buf, imageo_buf, clkernelD_buf, clkernelR_buf, clradius_buf;

		err = clGetPlatformIDs(1, &Pid, NULL);
		if (err < 0) {
			perror("Couldn't find any platforms");
			system("pause");
			exit(1);
		}
		err = clGetDeviceIDs(Pid, CL_DEVICE_TYPE_CPU, 1, &Did, NULL);
		if (err < 0) {
			perror("Couldn't find any devices");
			system("pause");
			exit(1);
		}
		Ctext = clCreateContext(NULL, 1, &Did, NULL, NULL, &err);
		if (err < 0) {
			perror("Couldn't create a context");
			system("pause");
			exit(1);
		}
		PH = fopen("acc.cl", "rb");
		if (PH == NULL) {
			perror("Couldn't find the program file");
			system("pause");
			exit(1);
		}
		fseek(PH, 0, SEEK_END);
		PS = ftell(PH);
		rewind(PH);
		PB = (char*)malloc(PS + 1);
		PB[PS] = '\0';
		fread(PB, sizeof(char), PS, PH);
		fclose(PH);

		pro = clCreateProgramWithSource(Ctext, 1, (const char**)&PB, &PS, &err);
		if (err < 0) {
			perror("Couldn't create the program");
			system("pause");
			exit(1);
		}
		free(PB);
		err = clBuildProgram(pro, 0, NULL, NULL, NULL, NULL);
		if (err < 0) {


			clGetProgramBuildInfo(pro, Did, CL_PROGRAM_BUILD_LOG,
				0, NULL, &LS);
			PL = (char*)malloc(LS + 1);
			PL[LS] = '\0';
			clGetProgramBuildInfo(pro, Did, CL_PROGRAM_BUILD_LOG,
				LS + 1, PL, NULL);
			printf("%s\n", PL);
			free(PL);
			system("pause");
			exit(1);
		}

		kernel1 = clCreateKernel(pro, "test", &err);
		if (err < 0) {
			perror("Couldn't create the kernel");
			system("pause");
			exit(1);
		}
		cl_command_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
		Cqueue = clCreateCommandQueue(Ctext, Did, properties[1], &err);
		if (err < 0) {
			perror("Couldn't create the command queue");
			exit(1);
		}
		// create command queue :
		
		clGetDeviceInfo(Did, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &size, 0);
		//printf("\n %d \n", size);
		//printf("size = %d \n", sizeof(int) * z * oheight);

		imagei_buf = clCreateBuffer(Ctext, CL_MEM_READ_ONLY, sizeof(uchar)*width * oheight, NULL, &err);
		if (err < 0) {
			perror("Couldn't create a buffer object first");
			printf("%d", err);
			system("pause");
			exit(1);
		}
		clkernelD_buf = clCreateBuffer(Ctext, CL_MEM_READ_ONLY, sizeof(double)*dsize, NULL, &err);
		if (err < 0) {
			perror("Couldn't create a buffer object A");
			system("pause");
			exit(1);
		}
		clkernelR_buf = clCreateBuffer(Ctext, CL_MEM_READ_ONLY, sizeof(double)*rsize, NULL, &err);
		if (err < 0) {
			perror("Couldn't create a buffer object B");
			system("pause");
			exit(1);
		}
		clradius_buf = clCreateBuffer(Ctext, CL_MEM_READ_ONLY, sizeof(int)* 3, NULL, &err);
		if (err < 0) {
			perror("Couldn't create a buffer object C");
			system("pause");
			exit(1);
		}
		imageo_buf = clCreateBuffer(Ctext, CL_MEM_WRITE_ONLY, sizeof(uchar)*(width * oheight), NULL, &err);
		if (err < 0) {
			perror("Couldn't create a res_buffer");
			system("pause");
			exit(1);
		}

		clEnqueueWriteBuffer(Cqueue, imagei_buf, CL_TRUE, 0, sizeof(uchar)*width * oheight, imagei, 0, NULL, 0);
		clEnqueueWriteBuffer(Cqueue, clkernelD_buf, CL_TRUE, 0, sizeof(double)*dsize, bilateral->clkernelD, 0, NULL, 0);
		clEnqueueWriteBuffer(Cqueue, clkernelR_buf, CL_TRUE, 0, sizeof(double)*rsize, bilateral->clkernelR, 0, NULL, 0);
		clEnqueueWriteBuffer(Cqueue, clradius_buf, CL_TRUE, 0, sizeof(int)* 3, radiusa, 0, NULL, 0);

		err = clSetKernelArg(kernel1, 0, sizeof(cl_mem), &imagei_buf);
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
		err = clSetKernelArg(kernel1, 4, sizeof(cl_mem), &imageo_buf);
		if (err < 0) {
			perror("Couldn't set the kernel argument");
			exit(1);
		}
		cl_event evt;


		WUPK = width * oheight;

		err = clEnqueueNDRangeKernel(Cqueue, kernel1, 1, NULL, &WUPK, NULL, 0, NULL, &evt);
		if (err < 0) {
			perror("Couldn't enqueue the kernel execution command");
			system("pause");
			exit(1);
		}
		status = clWaitForEvents(1, &evt);
		status = clGetEventProfilingInfo(evt,
			CL_PROFILING_COMMAND_START,
			sizeof(cl_ulong),
			&startTime,
			0);


		status = clGetEventProfilingInfo(evt,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			&endTime,
			0);


		//printf("%f , %f \n", 1e-9*startTime, 1e-9*endTime);
		std::cout << "Kernel Execution time: " << (1e-9 *endTime - 1e-9 *startTime) << std::endl;

		err = clEnqueueReadBuffer(Cqueue, imageo_buf, CL_TRUE, 0, sizeof(uchar)*(width * oheight), imagei, 0, NULL, &evt);
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
		
		clReleaseMemObject(imagei_buf);
		clReleaseMemObject(clkernelD_buf);
		clReleaseMemObject(clkernelR_buf);
		clReleaseMemObject(clradius_buf);
		clReleaseMemObject(imageo_buf);
		clReleaseKernel(kernel1);
		clReleaseCommandQueue(Cqueue);
		clReleaseProgram(pro);
		clReleaseContext(Ctext);
		

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
