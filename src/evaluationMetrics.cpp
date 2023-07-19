#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <utility>
#include <opencv2/opencv.hpp>
#include "evaluationMetrics.h"

using namespace std;
using namespace std::chrono;
using namespace cv;

double getPSNR(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2
    Scalar s = sum(s1);         // sum elements per channel
    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    double  mse =sse /(double)(I1.channels() * I1.total());
    double psnr = 10.0*log10((255*255)/mse);
    return psnr;
}

double getMSE(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    // s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2
    // Scalar s = sum(s1);         // sum elements per channel
    Scalar s = mean(s1);         // sum elements per channel
    // double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    // double  mse =sse /(double)(I1.channels() * I1.total());
    double  mse = s[0] /(double)(I1.total());

    return mse;

    // double diff = 0;
    // for(int i = 0; i < I1.rows; i++) {
    //     for(int j = 0; j < I1.cols; j++) {
    //         if(i + j < 200) {
    //         cout << to_string(I1.at<uchar>(i, j)) << endl;
    //         cout << to_string(I2.at<uchar>(i, j)) << endl;
    //         }
            
    //         diff += abs(I1.at<uchar>(i, j) - I2.at<uchar>(i, j));
    //     }
    // }

    // diff *= diff;

    // cout << to_string(diff) << endl;

    // return (diff / I1.total());
}

Scalar getMSSIM( const Mat& i1, const Mat& i2)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /***************************** INITS **********************************/
    int d     = CV_32F;
    Mat I1, I2;
    i1.convertTo(I1, d);           // cannot calculate on one byte large values
    i2.convertTo(I2, d);
    Mat I2_2   = I2.mul(I2);        // I2^2
    Mat I1_2   = I1.mul(I1);        // I1^2
    Mat I1_I2  = I1.mul(I2);        // I1 * I2
    /*************************** END INITS **********************************/
    Mat mu1, mu2;   // PRELIMINARY COMPUTING
    GaussianBlur(I1, mu1, Size(11, 11), 1.5);
    GaussianBlur(I2, mu2, Size(11, 11), 1.5);
    Mat mu1_2   =   mu1.mul(mu1);
    Mat mu2_2   =   mu2.mul(mu2);
    Mat mu1_mu2 =   mu1.mul(mu2);
    Mat sigma1_2, sigma2_2, sigma12;
    GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;
    GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;
    GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;
    Mat t1, t2, t3;
    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))
    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))
    Mat ssim_map;
    divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;
    Scalar mssim = mean( ssim_map ); // mssim = average of ssim map
    return mssim;
}

void testQuality(Mat &mainImage, Mat &testImage) {
    double mse = getMSE(mainImage, testImage);
    cout << "MSE value: " << mse << endl;

    Scalar ssim = getMSSIM(mainImage, testImage);
    cout << "SSIM value: " << ssim[0] << endl;

    double psnr = getPSNR(mainImage, testImage);
    cout << "PSNR value: " << psnr << endl;
}

double calcAvgDiff(Mat& image1, Mat& image2) {
    // Ensure that the images have the same dimensions
    if (image1.rows != image2.rows || image1.cols != image2.cols) {
        cerr << "Error: images must have the same dimensions." << endl;
        return 1;
    }

    // Calculate the maximum absolute difference between the two images
    double sum_diff = 0;
    for (int i = 0; i < image1.rows; i++) {
        for (int j = 0; j < image1.cols; j++) {
            sum_diff += abs(image1.at<uchar>(i, j) - image2.at<uchar>(i, j));
        }
    }

    return sum_diff/image1.total();
}

double calcDiffSum(Mat& image1, Mat& image2) {
    // Ensure that the images have the same dimensions
    if (image1.rows != image2.rows || image1.cols != image2.cols) {
        cerr << "Error: images must have the same dimensions." << endl;
        return 1;
    }

    // Calculate the maximum absolute difference between the two images
    double sum_diff = 0;
    for (int i = 0; i < image1.rows; i++) {
        for (int j = 0; j < image1.cols; j++) {
            sum_diff += abs(image1.at<uchar>(i, j) - image2.at<uchar>(i, j));
        }
    }

    return sum_diff;
}

template <typename Func, typename... Args>
double measureExecutionTime(Func&& func, Args&&... args) {
    auto start = high_resolution_clock::now();
    invoke(forward<Func>(func), forward<Args>(args)...);
    auto end = high_resolution_clock::now();
    return duration_cast<nanoseconds>(end - start).count() / 1000000.0;
}

double getMSEBaseline(const Mat& I1, const Mat& I2, int length)
{
    // Mat s1;
    // absdiff(I1, I2, s1);       // |I1 - I2|
    // // s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    // s1 = s1.mul(s1);           // |I1 - I2|^2
    // // Scalar s = sum(s1);         // sum elements per channel
    // Scalar s = mean(s1);         // sum elements per channel
    // // double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    // // double  mse =sse /(double)(I1.channels() * I1.total());
    // double  mse = s[0] /(double)(I1.total());

    // return mse;

    int counter = 0;
    int pixels = 0;

    double diff = 0;
    for(int i = 0; i < I1.rows; i++) {
        for(int j = 0; j < I1.cols; j++) {
            if(counter == length) {
                break;
            }
            diff += abs(I1.at<uchar>(i, j) - I2.at<uchar>(i, j));
            counter += 2;
            pixels++;
        }
    }

    diff *= diff;

    return (diff / pixels);
}



double getPSNRBaseline(const Mat& I1, const Mat& I2, int length)
{
    double mse = getMSEBaseline(I1, I2, length);
    double psnr = 10.0*log10((255*255)/mse);
    return psnr;
}

double getMSEOption2(const Mat& I1, const Mat& I2, int length)
{
    // Mat s1;
    // absdiff(I1, I2, s1);       // |I1 - I2|
    // // s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    // s1 = s1.mul(s1);           // |I1 - I2|^2
    // // Scalar s = sum(s1);         // sum elements per channel
    // Scalar s = mean(s1);         // sum elements per channel
    // // double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    // // double  mse =sse /(double)(I1.channels() * I1.total());
    // double  mse = s[0] /(double)(I1.total());

    // return mse;

    int counter = 0;
    int pixels = 0;

    double diff = 0;
    for(int i = 0; i < I1.rows; i++) {
        for(int j = 0; j < I1.cols; j++) {
            if(counter == length) {
                break;
            }
            diff += abs(I1.at<uchar>(i, j) - I2.at<uchar>(i, j));
            counter++;
            pixels++;
        }
    }

    diff *= diff;

    return (diff / pixels);
}



double getPSNROption2(const Mat& I1, const Mat& I2, int length)
{
    double mse = getMSEOption2(I1, I2, length);
    double psnr = 10.0*log10((255*255)/mse);
    return psnr;
}

void testQualityBaseline(Mat &mainImage, Mat &testImage, int length) {
    double mse = getMSEBaseline(mainImage, testImage, length);
    cout << "MSE value: " << mse << endl;

    double psnr = getPSNRBaseline(mainImage, testImage, length);
    cout << "PSNR value: " << psnr << endl;

    double sumDiff = calcDiffSum(mainImage, testImage);
    cout << "Sum of differences: " << sumDiff << endl;

    double avgDiff = calcAvgDiff(mainImage, testImage);
    cout << "Avg differenc: " << avgDiff << endl;

    Scalar ssim = getMSSIM(mainImage, testImage);
    cout << "SSIM value: " << ssim[0] << endl;

}

void testQualityOption2(Mat &mainImage, Mat &testImage, int length) {
    double mse = getMSEOption2(mainImage, testImage, length);
    cout << "MSE value: " << mse << endl;

    double sumDiff = calcDiffSum(mainImage, testImage);
    cout << "Sum of differences: " << sumDiff << endl;

    double avgDiff = calcAvgDiff(mainImage, testImage);
    cout << "Avg differenc: " << avgDiff << endl;

    double psnr = getPSNROption2(mainImage, testImage, length);
    cout << "PSNR value: " << psnr << endl;

    Scalar ssim = getMSSIM(mainImage, testImage);
    cout << "SSIM value: " << ssim[0] << endl;
}

string getDBImage(int capacity, string dbImagesPath) {
    // Open the file
    ifstream inputFile(dbImagesPath);
    if (!inputFile) {
        cerr << "Error opening file." << endl;
        return "";
    }

    // Read the file line by line
    string line;
    while (getline(inputFile, line)) {
        stringstream ss(line);
        string token;

        // Split the line into integer and text using comma as a delimiter
        if (getline(ss, token, ',')) {
            int number;
            istringstream(token) >> number;

            // Compare the number with the temp value
            if (number >= capacity) {
                // Retrieve the corresponding text
                if (getline(ss, token)) {
                    return token;
                }
            }
        }
    }

    // Close the file
    inputFile.close();

    return "";
}
