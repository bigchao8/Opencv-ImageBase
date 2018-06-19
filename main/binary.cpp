#include "imagebase.hpp"
using namespace std;
using namespace cv;

void FixedThreshold::Binarization(Mat &src)
{
    Mat src_gray = src.clone();
    threshold(src_gray, src, 50, 255, 0);
    src_gray.release();
    ipinfo->info("FixedThreshold min:50 max: 255");
    if (debug)
    {
        imwrite("threshold.jpg", src);
    }
}

void OTsuThreshold::Binarization(Mat &src)
{
    Mat gray = src.clone();
    threshold(gray, src, 0, 255, CV_THRESH_OTSU);
    gray.release();
    ipinfo->info("OTsuThreshold min:0 max: 255");
    if (debug)
    {
        imwrite("OTsuThreshold.jpg", src);
    }
}

void Adaptive::Binarization(Mat &src)
{
    Mat dst = src.clone();
    cvtColor(src, dst, CV_BGR2GRAY);
    adaptiveThreshold(dst, dst, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 25, 10);
    threshold(dst, src, 180, 255, 1);
    dst.release();
    ipinfo->info("Adaptive blockSize:25, C :10");
    if (debug)
    {
        imwrite("Adaptive.jpg", src);
    }
}
void Floodfill::Binarization(Mat &src)
{
    Point p1;
    p1.x = src.rows / 2.0;
    p1.y = src.cols / 2.0;
    Rect ccomp;

    floodFill(src, p1, Scalar(255, 255, 255), &ccomp,
              Scalar(20, 20, 20), Scalar(20, 20, 20));
    ipinfo->info("fist floodFill ");

    floodFill(src, Point(0, 0), Scalar(255, 255, 255), &ccomp,
              Scalar(20, 20, 20), Scalar(20, 20, 20));
    ipinfo->info("second floodFill ");

    floodFill(src, Point(src.cols - 10, 0), Scalar(255, 255, 255), &ccomp,
              Scalar(20, 20, 20), Scalar(20, 20, 20));
    ipinfo->info("third floodFill ");

    floodFill(src, Point(0, src.rows - 10), Scalar(255, 255, 255), &ccomp,
              Scalar(20, 20, 20), Scalar(20, 20, 20));
    ipinfo->info("fourth floodFill ");

    floodFill(src, Point(src.cols - 10, src.rows - 10), Scalar(255, 255, 255), &ccomp,
              Scalar(20, 20, 20), Scalar(20, 20, 20));
    ipinfo->info("fifth floodFill ");
    if (debug)
    {
        imwrite("floodFill.jpg", src);
    }
}
