#ifndef QUALITY_CHECKER
#define QUALITY_CHECKER

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

double getPSNR(const Mat& I1, const Mat& I2);

Scalar getMSSIM( const Mat& i1, const Mat& i2);

void testQuality(Mat &mainImage, Mat &testImage);

void testQualityBaseline(Mat &mainImage, Mat &testImage, int length);

void testQualityOption2(Mat &mainImage, Mat &testImage, int length);

void calculateVarianceMetric(Mat &image);

void calcAvgMatrix(Mat& input_matrix);

double calcMaxDiff(Mat& image1, Mat& image2);

string getDBImage(int capacity, string dbImagesPath);

#endif