
// File: scannerLite.cpp
// An OpenCV program implementing the recognition feature of the app "CamScanner".
// It extracts the main document object from an image and adjust it to A4 size.

#include "imagebase.hpp"
using namespace cv;
using namespace std;

int w_a4, h_a4;
RNG rng(12345);

// Function:Get the image size after affine.
// Function parameters:input vector type four Ponit2f.
void GcalcDstSize(const vector<cv::Point2f> &corners)
{
  int h1 = sqrt((corners[0].x - corners[2].x) * (corners[0].x - corners[2].x) +
                (corners[0].y - corners[2].y) * (corners[0].y - corners[2].y));
  int h2 = sqrt((corners[1].x - corners[3].x) * (corners[1].x - corners[3].x) +
                (corners[1].y - corners[3].y) * (corners[1].y - corners[3].y));
  h_a4 = MAX(h1, h2);

  int w1 = sqrt((corners[0].x - corners[1].x) * (corners[0].x - corners[1].x) +
                (corners[0].y - corners[1].y) * (corners[0].y - corners[1].y));
  int w2 = sqrt((corners[2].x - corners[3].x) * (corners[2].x - corners[3].x) +
                (corners[2].y - corners[3].y) * (corners[2].y - corners[3].y));
  w_a4 = MAX(w1, w2);
}

// Get edges of an image
// @param gray - grayscale input image
// @param canny - output edge image
void getCanny(Mat gray, Mat &canny)
{
  Mat thres;
  double high_thres = threshold(gray, thres, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU), low_thres = high_thres * 0.5;
  cv::Canny(gray, canny, low_thres, high_thres);
}

struct Line
{
  Point _p1;
  Point _p2;
  Point _center;

  Line(Point p1, Point p2)
  {
    _p1 = p1;
    _p2 = p2;
    _center = Point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
  }
};

bool cmp_y(const Line &p1, const Line &p2)
{
  return p1._center.y < p2._center.y;
}

bool cmp_x(const Line &p1, const Line &p2)
{
  return p1._center.x < p2._center.x;
}

// Compute intersect point of two lines l1 and l2
// @param l1
// @param l2
// @return Intersect Point
Point2f computeIntersect(Line l1, Line l2)
{
  int x1 = l1._p1.x, x2 = l1._p2.x, y1 = l1._p1.y, y2 = l1._p2.y;
  int x3 = l2._p1.x, x4 = l2._p2.x, y3 = l2._p1.y, y4 = l2._p2.y;
  if (float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4))
  {
    Point2f pt;
    pt.x = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / d;
    pt.y = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / d;
    return pt;
  }
  return Point2f(-1, -1);
}

// Function:The document edge finds the main flow.
// Function parameters:input Mat type picture.
int Saturation::Edgedetection(Mat &img)
{
  ipinfo->info("saturation start ");

  // resize input image to img_proc to reduce computation
  Mat img_proc, canny;
  int w = img.size().width, h = img.size().height, min_w = 200;
  double scale = min(10.0, w * 1.0 / min_w);

  int w_proc = w * 1.0 / scale, h_proc = h * 1.0 / scale;
  resize(img, img_proc, Size(w_proc, h_proc));
  ipinfo->info(" src image resize scale {}", scale);

  GaussianBlur(img_proc, img_proc, Size(1, 1), 0, 0);
  ipinfo->info("GaussianBlur successful Size (1,1)");

  // Separate the paper with high saturation.
  Mat hsv(img_proc.size(), CV_8U, Scalar(0));
  Mat tmpH1(img_proc.size(), CV_8U, Scalar(0));
  Mat tmpH4(img_proc.size(), CV_8U, Scalar(0));
  Mat tmpH2(img_proc.size(), CV_8U, Scalar(0));
  Mat tmpH3(img_proc.size(), CV_8U, Scalar(0));
  blur(img_proc, img_proc, Size(3, 3)); // Gaussian blur
  ipinfo->info("blur successful Size(3, 3)");
  // Convert BGR image to HSV space.
  cvtColor(img_proc, hsv, CV_BGR2HSV);
  ipinfo->info("cvtColor CV_BGR2HSV successful");

  vector<Mat> mv;
  split(hsv, mv); //There are 3 channels

  inRange(mv[0], Scalar(35, 0.0, 0, 0), Scalar(77, 0.0, 0, 0), tmpH1);
  inRange(mv[1], Scalar(43.0, 0.0, 0, 0), Scalar(255, 0.0, 0, 0), tmpH2);
  inRange(mv[2], Scalar(46, 0.0, 0, 0), Scalar(255.0, 0.0, 0, 0), tmpH3);

  getCanny(tmpH2, canny);
  Canny(canny, canny, 50, 200, 3);
  if (debug)
  {
    imwrite("scanner_canny.jpg",canny);
  }
  ipinfo->info("Get canny successful");

  // Edge line makes edge line, but it will get more interference line
  Mat element1 = getStructuringElement(MORPH_RECT, Size(2, 2));
  dilate(canny, canny, element1);
  ipinfo->info("dilate canny successful");

  // The external rectangle is used to determine the range s of the hoff line.
  Mat threshold_output;
  vector<vector<Point>> contours;
  vector<Vec4i> hierarchy;

  /// Use Threshold to detect edges.
  threshold(canny, canny, 100, 255, THRESH_BINARY);
  ipinfo->info("Threshold detection edge.");
  /// Find the outline
  findContours(canny, contours, hierarchy, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
  ipinfo->info("findContours ");
  if ( contours.size() == 0 )
  {
    return false;
  }
  /// The polygon approximates the contour + gets the rectangle and the circular bounding box.
  vector<vector<Point>> contours_poly(contours.size());
  vector<Rect> boundRect(contours.size());
  vector<Point2f> center(contours.size());
  vector<float> radius(contours.size());

  for (int i = 0; i < contours.size(); i++)
  {
    approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
    boundRect[i] = boundingRect(Mat(contours_poly[i]));
  }
  ipinfo->info("approxPolyDp");

  /// Draw the outline of the polygon and the surrounding rectangle.
  Mat drawing = Mat::zeros(canny.size(), CV_8UC3);

  double maxarea = contourArea(contours[0]);
  int index;
  for (int i = 0; i < contours.size(); i++)
  {
    Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    drawContours(drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
    rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
    // Record the maximum rectangle in general case the maximum rectangle is valid content.
    double area = contourArea(contours[i]);
    if (maxarea <= area)
    {
      index = i;
      maxarea = area;
    }
  }
  ipinfo->warn("Maximum rectangular area and location. {} : {}", maxarea, index);

  // extract lines from the edge image
  vector<Vec4i> lines;
  vector<Line> horizontals, verticals;
  HoughLinesP(canny, lines, 1, CV_PI / 180, w_proc / 3, w_proc / 3, 20);
  ipinfo->info(" HoughLinesp");

  int lsize = lines.size();
  auto t = lines.begin();
  for (size_t i = 0; i < lsize; i++)
  {
    Vec4i v = lines[i];

    // Determines whether the line is within the rectangle.
    if (0 > pointPolygonTest(contours[index], Point2f(v[0], v[1]), false) ||
        0 > pointPolygonTest(contours[index], Point2f(v[2], v[3]), false))
    {
      lines.erase(t + i);
      continue;
    }

    // x1 - x2 y1 - y2
    double delta_x = v[0] - v[2], delta_y = v[1] - v[3];
    Line l(Point(v[0], v[1]), Point(v[2], v[3]));
    // get horizontal lines and vertical lines respectively
    if (fabs(delta_x) > fabs(delta_y))
    { // Judge a straight line
      horizontals.push_back(l);
    }
    else
    {
      verticals.push_back(l);
    }
    line(img_proc, Point(v[0], v[1]), Point(v[2], v[3]), Scalar(0, 0, 255), 1, CV_AA);
  }

  if (horizontals.size() + verticals.size() < 2)
  {
    ipinfo->error("The edge line was not found!!");
    return false;
  }

  // edge cases when not enough lines are detected
  if (horizontals.size() < 2)
  {
    ipinfo->warn(" The horizontal line is less than two.: {}", horizontals.size());
    if (horizontals.size() == 0 || horizontals[0]._center.y > h_proc / 2)
    {
      // Draw edge lines by image size.
      horizontals.push_back(Line(Point(0, 0), Point(w_proc - 1, 0))); // The above line
    }
    if (horizontals.size() == 0 || horizontals[0]._center.y <= h_proc / 2)
    {
      horizontals.push_back(Line(Point(0, h_proc - 1), Point(w_proc - 1, h_proc - 1))); // The following line
    }
  }
  if (verticals.size() < 2)
  {
    ipinfo->warn(" The verticals line is less than two.: {}", verticals.size());
    if (verticals.size() == 0 || verticals[0]._center.x > w_proc / 2)
    {
      verticals.push_back(Line(Point(0, 0), Point(0, h_proc - 1))); // On the left side of the line
    }
    if (verticals.size() == 0 || verticals[0]._center.x <= w_proc / 2)
    {
      verticals.push_back(Line(Point(w_proc - 1, 0), Point(w_proc - 1, h_proc - 1))); // The right of the line
    }
  }

  // sort lines according to their center point 升序排序
  sort(horizontals.begin(), horizontals.end(), cmp_y);
  sort(verticals.begin(), verticals.end(), cmp_x);

  // Let's see if both sides of this line overlap.
  if (fabs(horizontals[0]._p1.y - horizontals[horizontals.size() - 1]._p1.y) < 80 &&
      abs(verticals[0]._p1.x - verticals[verticals.size() - 1]._p1.x) < 80)
  {
    ipinfo->error(" Do not find the suitable hoff line, the two lines overlap!");
    return false;
  }

  // Determine whether the horizontal hoff line is coincident, according to the center point, is the left or right overlap.
  if (fabs(horizontals[0]._p1.y - horizontals[horizontals.size() - 1]._p1.y) < 80)
  {

    ipinfo->warn("Horizontal lines overlap");
    // Mirror horizon lines
    double vly = img_proc.size().height - horizontals[0]._p1.y;
    // If it's the top line.
    if (horizontals[0]._center.y < h_proc / 2)
    {
      ipinfo->warn("The above horizontal lines overlap!");
      horizontals[horizontals.size() - 1]._p1.y = horizontals[0]._p1.y + vly;
      horizontals[horizontals.size() - 1]._p1.x = horizontals[0]._p1.x;

      horizontals[horizontals.size() - 1]._p2.y = horizontals[0]._p2.y + vly;
      horizontals[horizontals.size() - 1]._p2.x = horizontals[0]._p2.x;
    }

    // If it's the following line.
    if (horizontals[0]._center.y > h_proc / 2)
    {
      ipinfo->warn("The following horizontal lines overlap!");
      horizontals[horizontals.size() - 1]._p1.y = horizontals[0]._p1.y - vly;
      horizontals[horizontals.size() - 1]._p1.x = horizontals[0]._p1.x;

      horizontals[horizontals.size() - 1]._p2.y = horizontals[0]._p2.y - vly;
      horizontals[horizontals.size() - 1]._p2.x = horizontals[0]._p2.x;
    }
  }
  // Determine whether the hough line of vertical direction is coincident.
  if (fabs(verticals[0]._p1.x - verticals[verticals.size() - 1]._p1.x) < 80)
  {
    ipinfo->warn("verticals lines overlap");
    // Mirror vertical lines
    double vlx = img_proc.size().width - verticals[0]._p1.x;
    // On the left side of overlap
    if (verticals[0]._center.x < w_proc / 2)
    {
      ipinfo->warn("The vertical lines on the left overlap!");
      verticals[verticals.size() - 1]._p1.x = verticals[0]._p1.x + vlx;
      verticals[verticals.size() - 1]._p1.y = verticals[0]._p1.y;

      verticals[verticals.size() - 1]._p2.x = verticals[0]._p2.x + vlx;
      verticals[verticals.size() - 1]._p2.y = verticals[0]._p2.y;
    }
    // On the right side of the overlap
    if (verticals[0]._center.x > w_proc / 2)
    {
      ipinfo->warn("The vertical line on the right is overlapping!");
      verticals[verticals.size() - 1]._p1.x = verticals[0]._p1.x - vlx;
      verticals[verticals.size() - 1]._p1.y = verticals[0]._p1.y;

      verticals[verticals.size() - 1]._p2.x = verticals[0]._p2.x - vlx;
      verticals[verticals.size() - 1]._p2.y = verticals[0]._p2.y;
    }
  }

  line(img_proc, horizontals[0]._p1, horizontals[0]._p2, Scalar(0, 255, 0), 2, CV_AA);
  line(img_proc, horizontals[horizontals.size() - 1]._p1, horizontals[horizontals.size() - 1]._p2, Scalar(0, 255, 0), 2, CV_AA);

  line(img_proc, verticals[0]._p1, verticals[0]._p2, Scalar(255, 0, 0), 2, CV_AA);
  line(img_proc, verticals[verticals.size() - 1]._p1, verticals[verticals.size() - 1]._p2, Scalar(255, 255, 0), 2, CV_AA);
  //namedWindow("img_proc",0);
  //imshow("img_proc",img_proc);
  if (debug)
  {
    imwrite("scannerhfline.jpg", img_proc);
  }

  //waitKey(0);

  /* perspective transformation */
  vector<Point2f> dst_pts, img_pts;
  // corners of source image with the sequence [tl, tr, bl, br]
  img_pts.push_back(computeIntersect(horizontals[0], verticals[0]));
  img_pts.push_back(computeIntersect(horizontals[0], verticals[verticals.size() - 1]));
  img_pts.push_back(computeIntersect(horizontals[horizontals.size() - 1], verticals[0]));
  img_pts.push_back(computeIntersect(horizontals[horizontals.size() - 1], verticals[verticals.size() - 1]));

  // Determine the reasonable rectangle and return the original graph.
  if (fabs(img_pts[0].x - img_pts[1].x) < 60 || fabs(img_pts[0].y - img_pts[2].y) < 60 ||
      fabs(img_pts[1].y - img_pts[3].y) < 60 || fabs(img_pts[2].x - img_pts[3].x) < 60)
  {
    ipinfo->error("The unreasonable hoff lines!");
    ipinfo->error(" points : {} :{}", img_pts[0].x, img_pts[1].x);
    ipinfo->error(" points : {} :{}", img_pts[0].y, img_pts[2].y);
    ipinfo->error(" points : {} :{}", img_pts[1].y, img_pts[3].y);
    ipinfo->error(" points : {} :{}", img_pts[2].x, img_pts[3].x);
    return false;
  }

  for (int i = 0; i < img_pts.size(); i++)
  {
    ipinfo->info("The four points of affine are: x : {} y :{}", img_pts[i].x, img_pts[i].y);
  }

  // corners of destination image with the sequence [tl, tr, bl, br]
  GcalcDstSize(img_pts);

  dst_pts.push_back(Point(0, 0));
  dst_pts.push_back(Point(w_a4 * scale, 0));
  dst_pts.push_back(Point(0, h_a4 * scale));
  dst_pts.push_back(Point(w_a4 * scale, h_a4 * scale));

  Mat dst = Mat::zeros(h_a4 * scale, w_a4 * scale, CV_8UC3);
  ipinfo->info("The image size after affine is: Width:{} Height:{}", dst.size().width, dst.size().height);

  // convert to original image scale
  for (size_t i = 0; i < img_pts.size(); i++)
  {
    img_pts[i].x *= scale; // Amplification point coordinate
    img_pts[i].y *= scale;
  }

  // get transformation matrix
  Mat transmtx = getPerspectiveTransform(img_pts, dst_pts);
  ipinfo->info(" getPerspectiveTransform");

  // apply perspective transformation
  warpPerspective(img, dst, transmtx, dst.size());
  ipinfo->info(" warpPerspective");
  img = dst.clone();
  dst.release();
  if (debug)
  {
    imshow("Edgedetection.jpg", img);
  }

  return true;
}
