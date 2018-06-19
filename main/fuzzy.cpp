#include "imagebase.hpp"
#define MAX_FUZZYDEGREE 4.0

using namespace std;
using namespace cv;

void sharpenImage(const cv::Mat &image, cv::Mat &result)
{
    // Create and initialize the filter template.
    cv::Mat kernel(3, 3, CV_32F, cv::Scalar(0));
    kernel.at<float>(1, 1) = 5.0;
    kernel.at<float>(0, 1) = -1.0;
    kernel.at<float>(1, 0) = -1.0;
    kernel.at<float>(1, 2) = -1.0;
    kernel.at<float>(2, 1) = -1.0;

    result.create(image.size(), image.type());

    // The image is filtered.
    cv::filter2D(image, result, image.depth(), kernel);
}

void Sharpen::Fuzzyre(cv::Mat &src)
{
    ipinfo->info("sharpen start");
    Mat imagesource = src.clone();
    if (!imagesource.data)
    {
        ipinfo->error("Fuzzyre moudle loding image error!");
        exit(-1);
    }
    if (imagesource.size().height > imagesource.size().width)
    {
        resize(imagesource, imagesource, Size(600, 800));
    }
    else
    {
        resize(imagesource, imagesource, Size(800, 600));
    }
    ipinfo->info("image resize :{} x {} ", imagesource.size().width, imagesource.size().height);

    Mat imageGrey;
    cvtColor(imagesource, imageGrey, CV_RGB2GRAY);
    ipinfo->info("cvColor CV_RGB2GRAY ");

    Mat imageSobel;
    Laplacian(imageGrey, imageSobel, CV_16U);
    ipinfo->info("Laplacian CV_16U ");

    // The average grayscale of the image.
    double meanValue = mean(imageSobel)[0];
    ipinfo->warn(" The fuzzy detection index is :{} ", meanValue);

    if (meanValue < MAX_FUZZYDEGREE)
    {
        ipinfo->warn("The image is blurred!");

        sharpenImage(src, src);
        ipinfo->info("image blur finish");
    }
    if (debug)
    {
        imwrite("sharpen.jpg", src);
    }
    imagesource.release();
    imageGrey.release();
    imageSobel.release();
}