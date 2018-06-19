
#include <memory>
#include <set>
#include <cstdio>
#include <map>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "spdlog/spdlog.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
    cv::Mat src = cv::imread(argv[1]);

    float angle = atof(argv[2]);
    cv::Point2f center(src.cols / 2, src.rows / 2);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1);
    cv::Rect bbox = cv::RotatedRect(center, src.size(), angle).boundingRect();

    rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
    rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;

    cv::Mat dst;
    cv::warpAffine(src, dst, rot, bbox.size());

    namedWindow("anle", 0);
    imshow("anle", dst);
    waitKey(0);
    return 0;
}