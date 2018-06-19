#include "imagebase.hpp"
using namespace std;
using namespace cv;

#define GRAY_THRESH 150 // Two - valued kernel parameters.
#define HOUGH_VOTE 100  // Two - valued kernel parameters.

Mat Fouriermain(Mat &src)
{
    Mat srcImg = src.clone();
    cvtColor(srcImg, srcImg, CV_BGR2GRAY);
    if (srcImg.empty())
    {
        ipinfo->error("srcImg.empty() error!");
        exit(0);
    }

    Point center(srcImg.cols / 2, srcImg.rows / 2);

    Mat padded;
    int opWidth = getOptimalDFTSize(srcImg.rows);
    int opHeight = getOptimalDFTSize(srcImg.cols);
    copyMakeBorder(srcImg, padded, 0, opWidth - srcImg.rows, 0, opHeight - srcImg.cols, BORDER_CONSTANT, Scalar::all(0));

    Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
    Mat comImg;

    //Merge into a double-channel image
    merge(planes, 2, comImg);

    //Use the same image as input and output,
    //so that the results can fit in Mat well
    dft(comImg, comImg);

    //Compute the magnitude
    //planes[0]=Re(DFT(I)), planes[1]=Im(DFT(I))
    //magnitude=sqrt(Re^2+Im^2)
    split(comImg, planes);
    magnitude(planes[0], planes[1], planes[0]);

    //Switch to logarithmic scale, for better visual results
    //M2=log(1+M1)
    Mat magMat = planes[0];
    magMat += Scalar::all(1);
    log(magMat, magMat);

    //Crop the spectrum
    //Width and height of magMat should be even, so that they can be divided by 2
    //-2 is 11111110 in binary system, operator & make sure width and height are always even
    magMat = magMat(Rect(0, 0, magMat.cols & -2, magMat.rows & -2));

    //Rearrange the quadrants of Fourier image,
    //so that the origin is at the center of image,
    //and move the high frequency to the corners
    int cx = magMat.cols / 2;
    int cy = magMat.rows / 2;

    Mat q0(magMat, Rect(0, 0, cx, cy));
    Mat q1(magMat, Rect(0, cy, cx, cy));
    Mat q2(magMat, Rect(cx, cy, cx, cy));
    Mat q3(magMat, Rect(cx, 0, cx, cy));

    Mat tmp;
    q0.copyTo(tmp);
    q2.copyTo(q0);
    tmp.copyTo(q2);

    q1.copyTo(tmp);
    q3.copyTo(q1);
    tmp.copyTo(q3);

    //Normalize the magnitude to [0,1], then to[0,255]
    normalize(magMat, magMat, 0, 1, CV_MINMAX);
    Mat magImg(magMat.size(), CV_8UC1);
    magMat.convertTo(magImg, CV_8UC1, 255, 0);

    //Turn into binary image
    threshold(magImg, magImg, GRAY_THRESH, 255, CV_THRESH_BINARY);

    //Find lines with Hough Transformation
    vector<Vec2f> lines;
    float pi180 = (float)CV_PI / 180;
    Mat linImg(magImg.size(), CV_8UC3);
    HoughLines(magImg, lines, 1, pi180, HOUGH_VOTE, 0, 0);
    int numLines = lines.size();
    for (int l = 0; l < numLines; l++)
    {
        float rho = lines[l][0], theta = lines[l][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        pt1.x = cvRound(x0 + 1000 * (-b));
        pt1.y = cvRound(y0 + 1000 * (a));
        pt2.x = cvRound(x0 - 1000 * (-b));
        pt2.y = cvRound(y0 - 1000 * (a));
    }

    //Find the proper angel from the three found angels
    float angel = 0;
    float piThresh = (float)CV_PI / 90;
    float pi2 = CV_PI / 2;
    for (int l = 0; l < numLines; l++)
    {
        float theta = lines[l][1];
        if (abs(theta) < piThresh || abs(theta - pi2) < piThresh)
            continue;
        else
        {
            angel = theta;
            break;
        }
    }

    //Calculate the rotation angel
    //The image has to be square,
    //so that the rotation angel can be calculate right
    angel = angel < pi2 ? angel : angel - CV_PI;
    if (angel != pi2)
    {
        float angelT = srcImg.rows * tan(angel) / srcImg.cols;
        angel = atan(angelT);
    }
    float angelD = angel * 180 / (float)CV_PI;
    if (angelD > 45.0 || angelD < -45.0)
    {
        ipinfo->error("fourier angel error {}",angelD);
        angelD = 0.0;
    }
    ipinfo->info("the rotation angel to be applied:{}", angelD);

    //Rotate the image to recover
    Mat rotMat = getRotationMatrix2D(center, angelD, 1.0);
    Mat dstImg = Mat::ones(srcImg.size(), CV_8UC3);
    warpAffine(src, dstImg, rotMat, srcImg.size(), 1, 0, Scalar(255, 255, 255));
    srcImg.release();
    rotMat.release();

    return dstImg;
}
void Fourier::TiltCorrection(Mat &src)
{
    ipinfo->info("Fourier------------");
    src = Fouriermain(src).clone();
    ipinfo->info(src.channels());
    if(debug)
    {
        imwrite("Fourier.jpg",src);
    }
}
