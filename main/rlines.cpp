#include "imagebase.hpp"
using namespace std;
using namespace cv;

/* A function call */

void Rline(Mat &src, Mat &mask)
{
    Mat tmp1(src.size(), src.type());
    Mat tmp2(src.size(), src.type());
    Mat tmp3(src.size(), src.type());
    Mat tmp4(src.size(), src.type());
    Mat tmp(src.size(), src.type());
    int srcw = src.size().width;
    int srch = src.size().height;

    Vec3b pixm;
    pixm[0] = 255;
    pixm[1] = 255;
    pixm[2] = 255;
    if (1 == src.channels())
    {
        for (int i = 0; i < mask.rows - 1; ++i)
        {
            for (int j = 0; j < mask.cols - 1; ++j)
            {

                if (255 == mask.at<uchar>(i, j) || 254 == mask.at<uchar>(i, j) || 253 == mask.at<uchar>(i, j) ||
                    252 == mask.at<uchar>(i, j) || 251 == mask.at<uchar>(i, j) || 250 == mask.at<uchar>(i, j) ||
                    249 == mask.at<uchar>(i, j) || 248 == mask.at<uchar>(i, j) || 247 == mask.at<uchar>(i, j) ||
                    246 == mask.at<uchar>(i, j))
                {
                    src.at<uchar>(i, j) = 255;
                    if (i > 3 && j > 3 && i < srch - 3 && j < srcw - 3)
                    {
                        src.at<uchar>(i - 1, j - 1) = 255;
                        src.at<uchar>(i + 1, j + 1) = 255;
                        src.at<uchar>(i - 2, j - 2) = 255;
                        src.at<uchar>(i + 2, j + 2) = 255;
                        src.at<uchar>(i - 3, j - 3) = 255;
                        src.at<uchar>(i + 3, j + 3) = 255;
                    }
                }
            }
        }
    }
    else
    {

        for (int i = 0; i < mask.rows - 1; ++i)
        {
            for (int j = 0; j < mask.cols - 1; ++j)
            {

                if (255 == mask.at<uchar>(i, j) || 254 == mask.at<uchar>(i, j) || 253 == mask.at<uchar>(i, j) ||
                    252 == mask.at<uchar>(i, j) || 251 == mask.at<uchar>(i, j) || 250 == mask.at<uchar>(i, j) ||
                    249 == mask.at<uchar>(i, j) || 248 == mask.at<uchar>(i, j) || 247 == mask.at<uchar>(i, j) ||
                    246 == mask.at<uchar>(i, j))
                {
                    src.at<Vec3b>(i, j) = pixm;
                    if (i > 3 && j > 3 && i < srch - 3 && j < srcw - 3)
                    {
                        src.at<Vec3b>(i - 1, j - 1) = pixm;
                        src.at<Vec3b>(i + 1, j + 1) = pixm;
                        src.at<Vec3b>(i - 2, j - 2) = pixm;
                        src.at<Vec3b>(i + 2, j + 2) = pixm;
                        src.at<Vec3b>(i - 3, j - 3) = pixm;
                        src.at<Vec3b>(i + 3, j + 3) = pixm;
                    }
                }
            }
        }

        tmp.release();
        tmp1.release();
        tmp2.release();
        tmp3.release();
    }
}

Mat Line(Mat &src)
{

    // Check if mask is loaded fine
    if (!src.data)
        ipinfo->error("Opening loading mask error!!!");

    // resizing for practical reasons
    Mat rsz;
    Size size(src.cols, src.rows);
    resize(src, rsz, size);

    // Transform source mask to gray if it is not
    Mat gray;

    if (rsz.channels() == 3)
    {
        cvtColor(rsz, gray, CV_BGR2GRAY);
    }
    else
    {
        gray = rsz;
    }

    // Apply adaptiveThreshold at the bitwise_not of gray, notice the ~ symbol
    Mat bw;
    adaptiveThreshold(~gray, bw, 255, CV_ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);

    // Create the images that will use to extract the horizonta and vertical lines
    Mat horizontal = bw.clone();
    Mat vertical = bw.clone();

    int scale = 15; // play with this variable in order to increase/decrease the amount of lines to be detected

    // Specify size on horizontal axis
    int horizontalsize = horizontal.cols / scale;

    // Create structure element for extracting horizontal lines through morphology operations
    Mat horizontalStructure = getStructuringElement(MORPH_RECT, Size(horizontalsize, 1));

    // Apply morphology operations
    erode(horizontal, horizontal, horizontalStructure, Point(-1, -1));
    dilate(horizontal, horizontal, horizontalStructure, Point(-1, -1));

    // Specify size on vertical axis
    int verticalsize = vertical.rows / scale;

    // Create structure element for extracting vertical lines through morphology operations
    Mat verticalStructure = getStructuringElement(MORPH_RECT, Size(1, verticalsize));

    // Apply morphology operations
    erode(vertical, vertical, verticalStructure, Point(-1, -1));
    dilate(vertical, vertical, verticalStructure, Point(-1, -1));

    // create a mask which includes the tables
    Mat mask(src.rows, src.cols, CV_8UC3);
    mask = horizontal + vertical;
    horizontal.release();
    vertical.release();
    return mask;
}

void Opening::Rlines(Mat &src)
{
    ipinfo->info("Rlines start----");
    Mat mask = Line(src);

    Rline(src, mask);
    if (debug)
    {
        imwrite("linemask.jpg", mask);
        imwrite("Rline.jpg", src);
    }
    mask.release();
}
