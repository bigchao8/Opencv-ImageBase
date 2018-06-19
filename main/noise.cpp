#include "imagebase.hpp"
using namespace std;
using namespace cv;

void Gaussian::NoiseReduction(Mat &src)
{

    Mat tmp = src.clone();
    GaussianBlur(tmp, src, Size(5, 5), 0, 0);
    tmp.release();
    ipinfo->info("Gaussian Size(5,5)");
    if (debug)
    {
        imwrite("Gaussian.jpg", src);
    }
}

void Bilateral::NoiseReduction(Mat &src)
{
    Mat tmp = src.clone();
    bilateralFilter(tmp, src, 29, 29 * 2, 29 / 2);
    tmp.release();
    ipinfo->info("Bilateral Size(29,29*2)");
    if (debug)
    {
        imwrite("Bilateral.jpg", src);
    }
}

void Median::NoiseReduction(Mat &src)
{
    Mat tmp = src.clone();
    medianBlur(tmp, src, 3);
    tmp.release();
    ipinfo->info("Median Size(3)");
    if (debug)
    {
        imwrite("Median.jpg", src);
    }
}

void Average::NoiseReduction(Mat &src)
{
    Mat tmp = src.clone();
    blur(tmp, src, Size(5, 5), Point(-1, -1));
    tmp.release();
    ipinfo->info("Average Size(5,5) Point(-1,-1)");
    if (debug)
    {
        imwrite("Average.jpg", src);
    }
}

void Smooth::NoiseReduction(Mat &src)
{

    fastNlMeansDenoisingColored(src, src, 13, 13, 7, 21);
    ipinfo->info("fastNlMeansDenoisingColored Size(13,13),7,21");
    if (debug)
    {
        imwrite("fastNlMeansDenoisingColored.jpg", src);
    }
}
