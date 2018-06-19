#include "imagebase.hpp"
using namespace std;
using namespace cv;


// Normalizes a given image into a value range between 0 and 255.
Mat norm_0_255(const Mat &src);

//
// Calculates the TanTriggs Preprocessing as described in:
//
//      Tan, X., and Triggs, B. "Enhanced local texture feature sets for face
//      recognition under difficult lighting conditions.". IEEE Transactions
//
// Default parameters are taken from the paper.
//
Mat tan_triggs_preprocessing(InputArray src,
                             float alpha = 0.1,
                             float tau = 10.0,
                             float gamma = 0.2,
                             int sigma0 = 1,
                             int sigma1 = 2);

void TanTriggs::Shadow(Mat &src)
{
    // Load image & get skin proportions:
    Mat image = src.clone();

    // Calculate the TanTriggs Preprocessed image with default parameters:
    Mat preprocessed = tan_triggs_preprocessing(image);
    src = norm_0_255(preprocessed);
}


Mat norm_0_255(const Mat &src)
{
    // Create and return normalized image:
    Mat dst;
    switch (src.channels())
    {
    case 1:
        cv::normalize(src, dst, 0, 255, NORM_MINMAX, CV_8UC1);
        break;
    case 3:
        cv::normalize(src, dst, 0, 255, NORM_MINMAX, CV_8UC3);
        break;
    default:
        src.copyTo(dst);
        break;
    }
    return dst;
}

Mat tan_triggs_preprocessing(InputArray src,
                             float alpha,
                             float tau,
                             float gamma,
                             int sigma0,
                             int sigma1)
{

    // Convert to floating point:
    Mat X = src.getMat();
    X.convertTo(X, CV_32FC1);

    // Start preprocessing:
    Mat I;
    pow(X, gamma, I);
    // Calculate the DOG Image:
    {
        Mat gaussian0, gaussian1;
        // Kernel Size:
        int kernel_sz0 = (3 * sigma0);
        int kernel_sz1 = (3 * sigma1);
        // Make them odd for OpenCV:
        kernel_sz0 += ((kernel_sz0 % 2) == 0) ? 1 : 0;
        kernel_sz1 += ((kernel_sz1 % 2) == 0) ? 1 : 0;
        GaussianBlur(I, gaussian0, Size(kernel_sz0, kernel_sz0), sigma0, sigma0, BORDER_REPLICATE);
        GaussianBlur(I, gaussian1, Size(kernel_sz1, kernel_sz1), sigma1, sigma1, BORDER_REPLICATE);
        subtract(gaussian0, gaussian1, I);
    }

    {
        double meanI = 0.0;
        {
            Mat tmp;
            pow(abs(I), alpha, tmp);
            meanI = mean(tmp).val[0];
        }
        I = I / pow(meanI, 1.0 / alpha);
    }

    {
        double meanI = 0.0;
        {
            Mat tmp;
            pow(min(abs(I), tau), alpha, tmp);
            meanI = mean(tmp).val[0];
        }
        I = I / pow(meanI, 1.0 / alpha);
    }

    // Squash into the tanh:
    {
        Mat exp_x, exp_negx;
        exp(I / tau, exp_x);
        exp(-I / tau, exp_negx);
        divide(exp_x - exp_negx, exp_x + exp_negx, I);
        I = tau * I;
    }
    return I;
}
