#include "imagebase.hpp"

using namespace std;
using namespace cv;

Mat Preprocess1(const Mat &gray)
{
    // Sobel operator，X gradient
    Mat sobel;
    Sobel(gray, sobel, CV_8U, 1, 0, 3);
    // OTSU threshold
    Mat binary;
    threshold(sobel, binary, 0, 255, THRESH_OTSU + THRESH_BINARY);

    // Expansion and corrosion operation core setting.
    Mat element1 = getStructuringElement(MORPH_RECT, Size(30, 9));

    // The control height can control the expansion degree of the upper and lower rows
    // for example, 3 to 4 is stronger, but it can also result in omission.
    Mat element2 = getStructuringElement(MORPH_RECT, Size(24, 4));

    // Expand once to make the outline prominent.
    Mat dilate1;
    dilate(binary, dilate1, element2);

    // Corrosion once, remove details
    // table lines, etc. This is the vertical line.
    Mat erode1;
    erode(dilate1, erode1, element1);

    // Expand again to make the outline clear.
    Mat dilate2;
    dilate(erode1, dilate2, element2);

    sobel.release();
    binary.release();
    element1.release();
    element2.release();
    erode1.release();

    return dilate2;
}

double FindTextRegion1(const Mat &img)
{
    double max = .0;
    Mat approx;
    // Find the outline
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(img, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE, Point(0, 0));

    // Screen those small area
    for (int i = 0; i < contours.size(); i++)
    {
        // Calculate the area of ​​the current outline
        double area = contourArea(contours[i]);

        // Max X0 points
        if (max < area)
        {
            max = area;
        }

        // The area is less than 1000 all filtered out
        if (area < 1000)
            continue;

        // Contour approximate
        double epsilon = 0.001 * arcLength(contours[i], true);
        
        approxPolyDP(contours[i], approx, epsilon, true);

        // Find the smallest rectangle that may have direction
        RotatedRect rect = minAreaRect(contours[i]);

        // Calculate height and width
        int m_width = rect.boundingRect().width;
        int m_height = rect.boundingRect().height;

        // Filter those too thin rectangles, leaving flat
        if (m_height > m_width * 1.2)
            continue;
    }
    approx.release();
    return max;
}
double Interarea::Moire(Mat &src)
{
    double rectmax;
    Mat tmp;
    Mat srcImg = src.clone();

    if (srcImg.empty())
    {
        ipinfo->error("Moire read Mat error!");
        return -1;
    }

    Sobel(srcImg, srcImg, CV_8U, 1, 0, 3, 0.4, 128);

    resize(srcImg, tmp, Size(600, 800), 0, 0, INTER_AREA);

    resize(srcImg, srcImg, Size(600, 800));
    Mat img = srcImg.clone();

    Mat dst = srcImg - tmp;

    // The pretreatment of morphological
    // transform can find the contour of the rectangle.
    cvtColor(dst, dst, CV_BGR2GRAY);
    Mat dilation = Preprocess1(dst);

    // Locate and filter text areas.
    rectmax = FindTextRegion1(dilation);

    srcImg.release();
    img.release();
    dst.release();
    dilation.release();
    tmp.release();


    return rectmax;
}