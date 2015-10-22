#include "cv.h"
#include "cxcore.h"

class BilateralFilter{
	
public:
	/*
	IplImage * image;
	IplImage * rimage;
	*/

	double kernelRadius;
	double ** kernelD;
	double *gaussSimilarity;
	
	double *clkernelD;
	double *clkernelR;

	BilateralFilter( double sigmaD, double sigmaR,int *dsize,int *rsize);
	double gauss(double sigma, int x, int y);
	/*
	IplImage * runFilter();
	void apply(int i, int j);
	bool isInsideBoundaries(int m, int n);
	double similarity(int p, int s);
	double gauss(double sigma, int x, int y);
	double BilateralFilter::getSpatialWeight(int m, int n, int i, int j);
	*/
};