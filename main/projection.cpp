#include "imagebase.hpp"
using namespace std;
using namespace cv;

int kdst_hight; // The height of the final image.
int kdst_width; // The width of the final image.
int knum = 0;   // An array of values

// Function:Correction of distortion images
// Function parameters: Mat type input and output pictures
// Function can not be null，pointer picture memory automatically released
Mat Detect(const Mat &img, int);

// Function:Sort points in a straight line
// Function parameters:Point2f type input and output
void GetRecLine(Point2f dst[]);

// Function:Calculate the final image size
// Function parameters:vector<Point2f> type input
void CalcDstSize(const vector<Point2f> &corners);

// Function:Calculate the intersection of straight lines
// Function parameters:Enter the first straight line point2f array
// Enter the second straight line point2f array
// Output point2f array
// The first second parameter should be 2 arrays
// The output point2f should be 4 arrays
void GetCrossPoint(Point2f P1[], Point2f P2[], Point2f dst[]);

// Function:Adjust the order of the corners to where we need it
// Function parameters: input vector<Point2f> type Four points
// input Pint2f type Rectangular center point
void SortCorners(vector<Point2f> &corners, Point2f center);

// Function:Calculate the image four straight lines
// Function parameters:input Mat type picture.
// Input matrix picture.
// input rectangle point.
// output 8 straight line point.
void Getslope(Mat img, Mat Mask, vector<RotatedRect> rects, Point2f dst[]);

// Function:Calculate the image four straight lines
// Function parameters:input Mat type picture.
// Input matrix picture.
// input rectangle point.
// output 8 straight line point.
void Stextpositioning(Mat img, Mat Mask, vector<RotatedRect> rects, Point2f dst[]);

// Function:Morphology processing images
// Function parameters:Input Mat type picture.
// Function returns Mat type processed image
Mat Preprocess(const Mat &gray);

// Function: Get the text rectangle.
// Funcion parameters:Input morphology-processed Mat type image
// Function returns vector<RotatedRect> type rectangle
vector<RotatedRect> FindTextRegion(const Mat &img);

void Projection::Affine(Mat &src, int dv)
{
    Mat textImageSrc = src.clone();
    ipinfo->info("Affine start---------");

    if (textImageSrc.empty() || textImageSrc.channels() != 3)
    {
        ipinfo->error("Projecton read image error ! chanels: {}", textImageSrc.channels());
        exit(0);
    }
    src = Detect(textImageSrc, dv).clone();
    if (debug)
    {
        imwrite("Affine.jpg", src);
    }
}

double Distances(Point p1, Point p2)
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

Mat Detect(const Mat &img, int dv = 0)
{
    // Convert to grayscale.
    Mat gray; // Store grayscale images
    Mat Re(img.size(), img.type());
    Mat Re2(img.size(), img.type());
    Mat Rec(img.size(), img.type()); // Draw a text outline
    RotatedRect tmp;                 // Temporary storage of rectangular points
    Point2f Sp[2];
    Point2f Dp[2];
    Point2f rep[4];

    //Grayscale
    cvtColor(img, gray, CV_BGR2GRAY);
    ipinfo->info("CvtColor successful");

    // floodfill

    // The pretreatment of morphological
    // transform can find the contour of the rectangle.
    Mat dilation = Preprocess(gray);
    ipinfo->info("call Preprocess successful");

    // Locate and filter text areas.
    vector<RotatedRect> rects = FindTextRegion(dilation);
    ipinfo->info("call FindTextRegion successful");
    ipinfo->info(" Find rects size : {}", rects.size());
    if (rects.size() == 0)
    {
        ipinfo->error("No rectangle is found!");
        return img;
    }

    // Backups are used to find a straight line.
    vector<RotatedRect> rec(rects);

    // Draw the area rectangle.
    int size = rects.size();
    for (int i = 0; i < size; i++)
    {
        Point2f Po[4];
        tmp = rects.back();
        tmp.points(Po);
        rects.pop_back();

        for (int j = 0; j <= 3; j++)
        {
            line(Rec, Po[j], Po[(j + 1) % 4], Scalar(0, 255, 0), 2);
        }
    }
    Mat mk = Rec.clone();

    if (dv == SCANNER)
    {
        ipinfo->info("dv == Scanner");
        // Get all the points
        vector<Point> points;
        for (int i = 0; i < size; i++)
        {
            Point2f Po[4];
            tmp = rec.back();
            tmp.points(Po);
            rec.pop_back();
            for (int j = 0; j <= 3; j++)
            {
                points.push_back(Po[j]);
            }
        }

        if (points.size() == 0)
        {
            ipinfo->error("No rectangle is found!");
            return img;
        }

        RotatedRect minRect = minAreaRect(Mat(points));
        Point2f vertex[4];
        minRect.points(vertex);

        for (int i = 0; i < 4; i++)
        {
            rep[i] = vertex[i];
        }
    }
    if (dv == MOBILE)
    {
        ipinfo->info("dv == Mobele");
        // Calculate four lines
        Point2f ex_po[8];

        if (rec.size() == 0)
        {
            ipinfo->error("No rectangle is found!");
            return img;
        }
        //Getslope(Rec, mk, rec, ex_po);
        Stextpositioning(Rec, mk, rec, ex_po);
        ipinfo->info("Mobele getslope successful!");
        for (int i = 0; i < 8; i += 2)
        {
            ipinfo->info("The four linear coordinates are: x :{} y: {} ----- x: {} y: {}",
                         ex_po[i].x, ex_po[i].y, ex_po[i + 1].x, ex_po[i + 1].y);
            if (debug)
            {
                Mat tmp(img.size(),img.type());
                line(tmp, ex_po[i], ex_po[i + 1], Scalar(0, 0, 255), 4, 8);
                imwrite("rectline.jpg", tmp);
            }
        }

        // The right vertical line and the bottom edge.
        Sp[0].x = ex_po[0].x;
        Sp[0].y = ex_po[0].y;
        Sp[1].x = ex_po[1].x;
        Sp[1].y = ex_po[1].y;
        Dp[0].x = ex_po[2].x;
        Dp[0].y = ex_po[2].y;
        Dp[1].x = ex_po[3].x;
        Dp[1].y = ex_po[3].y;
        GetCrossPoint(Sp, Dp, rep);

        // Bottom line and left vertical line
        Sp[0].x = ex_po[2].x;
        Sp[0].y = ex_po[2].y;
        Sp[1].x = ex_po[3].x;
        Sp[1].y = ex_po[3].y;
        Dp[0].x = ex_po[4].x;
        Dp[0].y = ex_po[4].y;
        Dp[1].x = ex_po[5].x;
        Dp[1].y = ex_po[5].y;
        GetCrossPoint(Sp, Dp, rep);

        // Left vertical line and top horizontal line
        Sp[0].x = ex_po[6].x;
        Sp[0].y = ex_po[6].y;
        Sp[1].x = ex_po[7].x;
        Sp[1].y = ex_po[7].y;
        GetCrossPoint(Sp, Dp, rep);

        // Top and right vertical lines
        Dp[0].x = ex_po[0].x;
        Dp[0].y = ex_po[0].y;
        Dp[1].x = ex_po[1].x;
        Dp[1].y = ex_po[1].y;
        GetCrossPoint(Sp, Dp, rep);
    }

    if (dv == COMPUTER_SCREEN)
    {
        ipinfo->info("dv == Computer_screen");
        if (rec.size() == 0)
        {
            ipinfo->error("No rectangle is found!");
            return img;
        }

        // Calculate four lines
        Point2f ex_po[8];
        Stextpositioning(Rec, mk, rec, ex_po);
        for (int i = 0; i < 8; i += 2)
        {
            ipinfo->info("The four linear coordinates are: x :{} y: {} ----- x: {} y: {}",
                         ex_po[i].x, ex_po[i].y, ex_po[i + 1].x, ex_po[i + 1].y);
            if (debug)
            {
                Mat tmp(img.size(),img.type());
                line(tmp, ex_po[i], ex_po[i + 1], Scalar(0, 0, 255), 4, 8);
                imwrite("rectline.jpg", tmp);
            }
        }

        // The right vertical line and the bottom edge.
        Sp[0].x = ex_po[0].x;
        Sp[0].y = ex_po[0].y;
        Sp[1].x = ex_po[1].x;
        Sp[1].y = ex_po[1].y;
        Dp[0].x = ex_po[2].x;
        Dp[0].y = ex_po[2].y;
        Dp[1].x = ex_po[3].x;
        Dp[1].y = ex_po[3].y;
        GetCrossPoint(Sp, Dp, rep);

        // Bottom line and left vertical line
        Sp[0].x = ex_po[2].x;
        Sp[0].y = ex_po[2].y;
        Sp[1].x = ex_po[3].x;
        Sp[1].y = ex_po[3].y;
        Dp[0].x = ex_po[4].x;
        Dp[0].y = ex_po[4].y;
        Dp[1].x = ex_po[5].x;
        Dp[1].y = ex_po[5].y;
        GetCrossPoint(Sp, Dp, rep);

        // Left vertical line and top horizontal line
        Sp[0].x = ex_po[6].x;
        Sp[0].y = ex_po[6].y;
        Sp[1].x = ex_po[7].x;
        Sp[1].y = ex_po[7].y;
        GetCrossPoint(Sp, Dp, rep);

        // Top and right vertical lines
        Dp[0].x = ex_po[0].x;
        Dp[0].y = ex_po[0].y;
        Dp[1].x = ex_po[1].x;
        Dp[1].y = ex_po[1].y;
        GetCrossPoint(Sp, Dp, rep);
    }

    // Determine the location of the four vertices
    std::vector<cv::Point2f> corners;
    Point2f center(0, 0);
    center = Point2f(0, 0);

    for (int i = 0; i < 4; i++)
    {
        corners.push_back(rep[i]);
        ipinfo->info("Transformation point for x:{} y:{}", rep[i].x, rep[i].y);

        if (rep[i].x < -100 || rep[i].x > img.size().width * 1.15 || rep[i].y < -100 || rep[i].y > img.size().height * 1.15)
        {
            ipinfo->error(" Straight line intersection calculation error! {} {}", rep[i].x, rep[i].y);
            if (debug)
            {
                return img;
            }
            exit(-1);
        }
    }
    for (int i = 0; i < corners.size(); i++)
        center += corners[i];
    center *= (1. / corners.size());

    // Call the function to get the position
    // of the top left corner of the upper right corner
    // of the lower right corner of the lower left corner
    SortCorners(corners, center);

    corners[0].x = corners[0].x - 50;
    corners[0].y = corners[0].y - 50;
    corners[1].x = corners[1].x + 50;
    corners[1].y = corners[1].y - 50;
    corners[2].x = corners[2].x + 50;
    corners[2].y = corners[2].y + 50;
    corners[3].x = corners[3].x - 50;
    corners[3].y = corners[3].y + 50;

    // Get the last image size
    CalcDstSize(corners);

    // Affine transformation order is the upper
    //  left corner, upper right corner
    // lower right corner, lower left corner
    if (kdst_hight > img.rows * 1.2 || kdst_width > img.cols * 1.2)
    {
        ipinfo->error("warp point error.Please enable the debug!{} {}", kdst_hight, kdst_width);
        return img;
    }
    if (kdst_width < img.cols * 0.7 && MOBILE == dv)
    {
        ipinfo->error("warp point error.Please enable the debug!{} {}", kdst_hight, kdst_width);
        return img;
    }
    cv::Mat quad = cv::Mat::zeros(kdst_hight, kdst_width, CV_8UC3);
    std::vector<cv::Point2f> quad_pts;

    quad_pts.push_back(cv::Point2f(0, 0));
    quad_pts.push_back(cv::Point2f(quad.cols, 0));
    quad_pts.push_back(cv::Point2f(quad.cols, quad.rows));
    quad_pts.push_back(cv::Point2f(0, quad.rows));
    // Affine rectangle
    cv::Mat transmtx = cv::getPerspectiveTransform(corners, quad_pts);
    // Affine transformation
    cv::warpPerspective(img, quad, transmtx, quad.size(),1,0,Scalar(255,255,255));

    return quad;
}

bool x_sort(const Point2f &m1, const Point2f &m2)
{
    return m1.x < m2.x;
}

vector<RotatedRect> FindTextRegion(const Mat &img)
{
    vector<RotatedRect> rects;
    // Find the outline
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(img, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE, Point(0, 0));

    // Screen those small area
    for (int i = 0; i < contours.size(); i++)
    {
        // Calculate the area of ​​the current outline
        double area = contourArea(contours[i]);

        // The area is less than 1000 all filtered out
        if (area < 1000 || area > 20000)
            continue;

        // Contour approximate
        double epsilon = 0.001 * arcLength(contours[i], true);
        Mat approx;
        approxPolyDP(contours[i], approx, epsilon, true);

        // Find the smallest rectangle that may have direction
        RotatedRect rect = minAreaRect(contours[i]);

        // Calculate height and width
        int m_width = rect.boundingRect().width;
        int m_height = rect.boundingRect().height;

        // Filter those too thin rectangles, leaving flat
        if (m_height > m_width * 1.2)
            continue;

        // Eligible rect added to the rects collection
        rects.push_back(rect);
    }
    return rects;
}

void Getslope(Mat img, Mat Mask, vector<RotatedRect> rects, Point2f dst[])
{
    int k = 0;                            // Number of rectangle points
    RotatedRect tmp;                      // Used to store rectangle points
    Point2f pot[4096];                    // Save all the rectangle points
    Point2f rp[4096];                     // Save all the rectangle points
    int size = rects.size();              // The number of rectangles
    vector<vector<cv::Point>> contours;   // Temporary storage of rectangular points
    vector<vector<cv::Point>> f_contours; // Store outline rectangle
    vector<Point> points;                 // Save all rects points
    vector<Point> horizontal;             // Save the convex hull vertical line.
    vector<Point> vertical;               // Save the convex hull all vertical line.
    vector<Point> lvertical;              // Save the convex hull right vertical line.
    vector<Point> rvertical;              // Save the convex hull left vertical line.
    vector<Point> hl;
    vector<int> hull;
    ipinfo->info("getslope start");
    // Get all the points
    for (int i = 0; i < size; i++)
    {
        Point2f Po[4];
        tmp = rects.back();
        tmp.points(Po);
        rects.pop_back();
        for (int j = 0; j <= 3; j++)
        {
            pot[k] = Po[j];
            points.push_back(pot[k]);
            k++;
        }
    }

    ipinfo->info("get all points successful");
    convexHull(Mat(points), hull, true);
    ipinfo->info("convexHull successful");

    for (int i = 0; i < k; i++)
        circle(img, points[i], 3, Scalar(0, 0, 255), CV_FILLED, CV_AA);

    int hullcount = (int)hull.size();
    Point pt0 = points[hull[hullcount - 1]];
    ipinfo->info("convexHull points size : {}", hullcount);
    Point pt;
    // Meet the horizon
    for (int i = 0; i < hullcount; i++)
    {
        pt = points[hull[i]];
        if ((pt0.x - pt.x) != 0)
            if (0 == fabs((pt0.y - pt.y) / (pt0.x - pt.x)))
            {
                horizontal.push_back(pt0);
                horizontal.push_back(pt);
            }
            else // Vertical line
            {
                vertical.push_back(pt0);
                vertical.push_back(pt);
                line(img, pt0, pt, Scalar(255, 0, 255), 8);
            }
        pt0 = pt;
    }
    ipinfo->info("Fetch the horizon");

    // Find the right and left lines.
    for (int i = 0; i < vertical.size() - 1; i++)
    {
        // Find the right vertical line to be judged by the difference value greater than itself.
        if (fabs(vertical[i].x - vertical[i + 1].x) > (vertical[i].x * 1.5) &&
            vertical[i].x != 0)
        {
            lvertical.push_back(vertical[i]);
            ++i;
            for (; i < vertical.size(); i++)
            {
                rvertical.push_back(vertical[i]);
                if (fabs(vertical[i + 1].x - vertical[i].x) > vertical[i + 1].x * 1.5)
                {
                    for (; i < vertical.size() - 1; i++)
                    {
                        lvertical.push_back(vertical[i + 1]);
                    }
                }
            }
        }
        else
        {
            lvertical.push_back(vertical[i]);
        }
    }

    // Find the longest convex hull line on the left.
    int distance = 0, maxd = fabs(Distances(lvertical[0], lvertical[1]));
    for (int i = 0; i < lvertical.size() - 1; i++)
    {
        if (maxd < fabs(Distances(lvertical[i], lvertical[i + 1])))
        {
            maxd = fabs(Distances(lvertical[i], lvertical[i + 1]));
            distance = i;
        }
    }

    // if The original image meets the standard.

    ipinfo->info("To find the convex hull");
    if (fabs((points[hull[hullcount - 1]].y - points[hull[hullcount - 2]].y) /
             (points[hull[hullcount - 1]].x - points[hull[hullcount - 2]].x)) == 0 &&
            fabs((points[hull[0]].y - points[hull[hullcount - 1]].y) /
                 (points[hull[0]].x - points[hull[hullcount - 1]].x)) == 0 ||
        fabs((points[hull[hullcount - 1]].y - points[hull[hullcount - 2]].y) /
             (points[hull[hullcount - 1]].x - points[hull[hullcount - 2]].x)) == 0 &&
            fabs((points[hull[hullcount - 2]].y - points[hull[hullcount - 3]].y) /
                 (points[hull[hullcount - 2]].x - points[hull[hullcount - 3]].x)) == 0 ||
        fabs((points[hull[0]].y - points[hull[hullcount - 1]].y) /
             (points[hull[0]].x - points[hull[hullcount - 1]].x)) == 0 &&
            fabs((points[hull[hullcount - 2]].y - points[hull[hullcount - 3]].y) /
                 (points[hull[hullcount - 2]].x - points[hull[hullcount - 3]].x)) == 0)
    {
        ipinfo->info("Convex packages do not conform to the general flow of calls.");
        // Max X0 points
        int b = 0, max = pot[b].x;
        for (int i = 0; i < k; ++i)
        {
            if (max < pot[i].x)
            {
                max = pot[i].x;
                b = i;
            }
        }

        // Max Y0 points
        int s, b1 = 0, max_m = pot[b1].y;
        for (s = 0; s < k; ++s)
        {
            if (max_m < pot[s].y)
            {
                max_m = pot[s].y;
                b1 = s;
            }
        }

        // Min X0 points
        int a, mib = 0, min = pot[mib].x;
        for (a = 0; a < k; ++a)
        {
            if (min > pot[a].x)
            {
                min = pot[a].x;
                mib = a;
            }
        }

        // Min Y0 points
        int a1, mib1 = 0, min1 = pot[mib].y;
        for (a1 = 0; a1 < k - 1; ++a1)
        {
            if (min1 > pot[a1].y)
            {
                min1 = pot[a1].y;
                mib1 = a1;
            }
        }

        // Get the right two rectangles
        Mat Find;
        cvtColor(Mask, Mask, CV_BGR2GRAY);
        threshold(Mask, Find, 0, 255, THRESH_OTSU + THRESH_BINARY);
        findContours(Find, f_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

        int max_area = 0;
        int index;
        for (int i = 0; i < f_contours.size(); i++)
        {
            double tmparea = fabs(contourArea(f_contours[i]));
            if (tmparea > max_area)
            {
                index = i;
                max_area = tmparea;
            }
        }
        contours.push_back(f_contours[index]);

        // The first rectangle
        // drawContours(img, contours, 0, Scalar(255,0,255), 6);
        RotatedRect R2 = minAreaRect(contours.back());
        Point2f Rp1[4];
        R2.points(Rp1);
        // Calculate the appropriate rectangle line
        GetRecLine(Rp1);
        line(img, Rp1[0], Rp1[1], Scalar(255, 255, 0), 5);

        // The second rectangle
        vector<vector<Point>>::iterator it = f_contours.begin() + index;
        f_contours.erase(it);

        // Max rect
        int max_area1 = 0;
        int index1;
        for (int i = 0; i < f_contours.size(); i++)
        {
            double tmparea = fabs(contourArea(f_contours[i]));
            if (tmparea > max_area1)
            {
                index1 = i;
                max_area1 = tmparea;
            }
        }
        contours.pop_back();
        contours.push_back(f_contours[index1]);
        // drawContours(img, contours, -1, Scalar(255,0,255), 6);
        // Get the rectangle four points
        RotatedRect R1 = minAreaRect(contours.back());
        Point2f Rp[4];
        R1.points(Rp);

        // Calculate the appropriate rectangle line
        GetRecLine(Rp);
        // Right straight line
        dst[0].x = pot[b].x;
        dst[0].y = pot[b].y;
        dst[1].x = pot[b].x;
        dst[1].y = pot[b].y + 200;

        // Bottom line
        float bottom = pot[b1].y - Rp[1].y; // Straight line and bottom vertex distance
        dst[2].x = Rp[0].x;
        dst[2].y = Rp[0].y + bottom + 10;
        dst[3].x = Rp[1].x;
        dst[3].y = Rp[1].y + bottom + 10;
        ;

        // Left straight line
        dst[4].x = pot[mib].x;
        dst[4].y = pot[mib].y;
        dst[5].x = pot[mib].x;
        dst[5].y = pot[mib].y + 200;

        // Top straight line
        float top = pot[mib1].y - Rp1[1].y;
        dst[6].x = Rp1[0].x;
        dst[6].y = Rp1[0].y + top - 100;
        dst[7].x = Rp1[1].x;
        dst[7].y = Rp1[1].y + top - 100;
    }
    else
    {

        if (fabs(Distances(points[hull[hullcount - 1]], points[hull[hullcount - 2]])) >
                fabs(Distances(points[hull[0]], points[hull[hullcount - 1]])) &&
            fabs((points[hull[hullcount - 1]].y - points[hull[hullcount - 2]].y) / (points[hull[hullcount - 1]].x - points[hull[hullcount - 2]].x)) > 5)
        {
            pt = points[hull[hullcount - 2]];
            pt0 = points[hull[hullcount - 1]];
        }
        else
        {
            pt = points[hull[0]];
            pt0 = points[hull[hullcount - 1]];
        }
        if (fabs(Distances(points[hull[hullcount - 2]], points[hull[hullcount - 3]])) >
                fabs(Distances(points[hull[hullcount - 1]], points[hull[hullcount - 2]])) &&
            fabs((points[hull[hullcount - 2]].y - points[hull[hullcount - 3]].y) / (points[hull[hullcount - 2]].x - points[hull[hullcount - 3]].x)) > 5)
        {

            pt = points[hull[hullcount - 3]];
            pt0 = points[hull[hullcount - 2]];
        }

        Point v2;
        Point v1;
        Point pt1 = points[hull[hullcount - 2]];
        Point pt2 = points[hull[hullcount - 3]];
        double distance = pt1.y - pt2.y;

        for (int d = 0; d < hullcount - 2; d++)
        {
            pt1 = points[hull[hullcount - d - 2]];
            pt2 = points[hull[hullcount - d - 3]];
            if (pt1.y - pt2.y < 0)
            {

                if (pt1.y - pt2.y < distance)
                {
                    distance = pt1.y - pt2.y;
                    v1 = pt1;
                    v2 = pt2;
                }
            }
        }

        // Max X0 points
        int b = 0, max = pot[b].x;
        for (int i = 0; i < k; ++i)
        {
            if (max < pot[i].x)
            {
                max = pot[i].x;
                b = i;
            }
        }

        // Max Y0 points
        int s, b1 = 0, max_m = pot[b1].y;
        for (s = 0; s < k; ++s)
        {
            if (max_m < pot[s].y)
            {
                max_m = pot[s].y;
                b1 = s;
            }
        }

        // Min X0 points
        int a, mib = 0, min = pot[mib].x;
        for (a = 0; a < k; ++a)
        {
            if (min > pot[a].x)
            {
                min = pot[a].x;
                mib = a;
            }
        }

        // Min Y0 points
        int a1, mib1 = 0, min1 = pot[mib].y;
        for (a1 = 0; a1 < k - 1; ++a1)
        {
            if (min1 > pot[a1].y)
            {
                min1 = pot[a1].y;
                mib1 = a1;
            }
        }

        for (int i = 0; i < horizontal.size(); i++)
        {
            if (horizontal[i].y == (int)pot[mib1].y)
            {
                hl.push_back(horizontal[i]);
                hl.push_back(horizontal[i + 1]);
                hl.push_back(horizontal[i + 2]);
                hl.push_back(horizontal[i - 1]);
                break;
            }
        }

        // Get the right two rectangles
        Mat Find;
        cvtColor(Mask, Mask, CV_BGR2GRAY);
        threshold(Mask, Find, 0, 255, THRESH_OTSU + THRESH_BINARY);
        findContours(Find, f_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

        int max_area = 0;
        int index;
        for (int i = 0; i < f_contours.size(); i++)
        {
            double tmparea = fabs(contourArea(f_contours[i]));
            if (tmparea > max_area)
            {
                index = i;
                max_area = tmparea;
            }
        }
        contours.push_back(f_contours[index]);

        // The first rectangle
        // drawContours(img, contours, 0, Scalar(255,0,255), 6);
        RotatedRect R2 = minAreaRect(contours.back());
        Point2f Rp1[4];
        R2.points(Rp1);
        for (int j = 0; j <= 3; j++)
        {
            line(img, Rp1[j], Rp1[(j + 1) % 4], Scalar(255, 0, 255), 2);
        }

        // Calculate the appropriate rectangle line
        GetRecLine(Rp1);
        line(img, Rp1[0], Rp1[1], Scalar(255, 255, 0), 5);

        // The second rectangle
        vector<vector<Point>>::iterator it = f_contours.begin() + index;
        f_contours.erase(it);

        // Max rect
        int max_area1 = 0;
        int index1;
        for (int i = 0; i < f_contours.size(); i++)
        {
            double tmparea = fabs(contourArea(f_contours[i]));
            if (tmparea > max_area1)
            {
                index1 = i;
                max_area1 = tmparea;
            }
        }
        contours.pop_back();
        contours.push_back(f_contours[index1]);
        // drawContours(img, contours, -1, Scalar(255,0,255), 6);
        // Get the rectangle four points
        RotatedRect R1 = minAreaRect(contours.back());
        Point2f Rp[4];
        R1.points(Rp);
        for (int j = 0; j <= 3; j++)
        {
            line(img, Rp[j], Rp[(j + 1) % 4], Scalar(255, 180, 0), 2);
        }
        // Calculate the appropriate rectangle line
        GetRecLine(Rp);
        line(img, Rp[0], Rp[1], Scalar(255, 255, 0), 5);

        // Right straight line
        if (v2.x - v1.x > 0)
        {
            dst[0].x = v1.x + 50;
            dst[0].y = v1.y;
            dst[1].x = v2.x + 50;
            dst[1].y = v2.y;
        }
        else
        {

            dst[0].x = pot[b].x + 50;
            dst[0].y = pot[b].y;
            dst[1].x = pot[b].x - (pt0.x - pt.x) + 50 + (pt0.x - pt.x) / 10;
            dst[1].y = pot[b].y - (pt.y - pt0.y);
        }
        line(img, dst[0], dst[1], Scalar(0, 0, 255), 6, CV_AA);

        // Bottom line
        float bottom = pot[b1].y - Rp[1].y; // Straight line and bottom vertex distance
        dst[2].x = Rp[0].x;
        dst[2].y = Rp[0].y + bottom + 50;
        dst[3].x = Rp[1].x;
        dst[3].y = Rp[1].y + bottom + 50;

        // Left straight line
        dst[4].x = pt0.x - 50;
        dst[4].y = pt0.y;
        dst[5].x = pt.x - 50;
        dst[5].y = pt.y;

        /*
    if ( fabs(Distances(hl[0],hl[1])) > fabs(Distances(hl[0],hl[2])) && fabs(Distances(hl[0],hl[1])) > fabs(Distances(hl[0],hl[3])))
    {
        dst[6].x = hl[0].x;
        dst[6].y = hl[0].y;
        dst[7].x = hl[1].x;
        dst[7].y = hl[1].y ;
    }
    if (fabs(Distances(hl[0],hl[2])) > fabs(Distances(hl[0],hl[3])))
    {
        dst[6].x = hl[0].x;
        dst[6].y = hl[0].y ;
        dst[7].x = hl[2].x;
        dst[7].y = hl[2].y ;
    }else {
        dst[6].x = hl[0].x;
        dst[6].y = hl[0].y ;
        dst[7].x = hl[3].x;
        dst[7].y = hl[3].y ;
    }*/
        float top = pot[mib1].y - Rp1[1].y;
        dst[6].x = Rp1[0].x;
        dst[6].y = Rp1[0].y + top - 100;
        dst[7].x = Rp1[1].x;
        dst[7].y = Rp1[1].y + top - 100;
    }
}

void Stextpositioning(Mat img, Mat Mask, vector<RotatedRect> rects, Point2f dst[])
{
    int k = 0;                            //Number of rectangle points
    RotatedRect tmp;                      //Used to store rectangle points
    Point2f pot[4096];                    //Save all the rectangle points
    int size = rects.size();              //The number of rectangles
    vector<vector<cv::Point>> contours;   //Temporary storage of rectangular points
    vector<vector<cv::Point>> f_contours; //Store outline rectangle
    vector<vector<cv::Point>> tf_contours;
    vector<RotatedRect> rectbk(rects);
    Mat Rec1(img.size(), img.type());
    Mat Rec2(img.size(), img.type());

    // Calculate the average Angle of the rectangle.
    float rectangle = .0;
    int zersdex = 0;
    for (int i = 0; i < size; i++)
    {
        Point2f Po[4];
        tmp = rectbk.back();
        tmp.points(Po);
        rectbk.pop_back();
        float an = tmp.angle;
        if (an > -46.0) // Eliminate too much Angle.
        {
            rectangle += tmp.angle;
            zersdex++;
        }
    }
    float avgrect = rectangle / (size - zersdex);

    // Get all the points
    for (int i = 0; i < size; i++)
    {
        Point2f Po[4];
        tmp = rects.back();
        tmp.points(Po);
        rects.pop_back();
        float an = tmp.angle;
        for (int j = 0; j <= 3; j++)
        {
            pot[k] = Po[j];
            k++;
            if (i < size / 2 && an >= avgrect * 1.5)
            {
                line(Rec1, Po[j], Po[(j + 1) % 4], Scalar(0, 255, 0), 2); // Calculated separately
            }
            if (i >= size / 2 && an >= avgrect * 1.5)
            {
                line(Rec2, Po[j], Po[(j + 1) % 4], Scalar(0, 255, 0), 2); // Calculated separately
            }
            if (debug)
            {
                Mat tmp1(img.size(), img.type());
                imwrite("Rec1.jpg", Rec1);
                imwrite("Rec2.jpg", Rec2);
            }
        }
    }

    // Max X0 points
    int i, b = 0, max = pot[b].x;
    for (i = 0; i < k; ++i)
    {
        if (max < pot[i].x)
        {
            max = pot[i].x;
            b = i;
        }
    }

    // Max Y0 points
    int s, b1 = 0, max_m = pot[b1].y;
    for (s = 0; s < k; ++s)
    {
        if (max_m < pot[s].y)
        {
            max_m = pot[s].y;
            b1 = s;
        }
    }

    // Min X0 points
    int a, mib = 0, min = pot[mib].x;
    for (a = 0; a < k; ++a)
    {
        if (min > pot[a].x)
        {
            min = pot[a].x;
            mib = a;
        }
    }

    // Min Y0 points
    int a1, mib1 = 0, min1 = pot[mib].y;
    for (a1 = 0; a1 < k - 1; ++a1)
    {
        if (min1 > pot[a1].y)
        {
            min1 = pot[a1].y;
            mib1 = a1;
        }
    }

    // Get the right two rectangles
    // 上半部分矩形
    Mat Find;
    cvtColor(Rec1, Rec1, CV_BGR2GRAY);
    threshold(Rec1, Find, 0, 255, THRESH_OTSU + THRESH_BINARY);
    findContours(Find, f_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    int max_area = 0;
    int index;
    for (int i = 0; i < f_contours.size(); i++)
    {
        double tmparea = fabs(contourArea(f_contours[i]));
        if (tmparea > max_area)
        {
            index = i;
            max_area = tmparea;
        }
    }
    // 最大矩形
    contours.push_back(f_contours[index]);

    // The first rectangle
    // drawContours(img, contours, 0, Scalar(255,0,255), 6);
    RotatedRect R2 = minAreaRect(contours.back());
    Point2f Rp1[4];
    R2.points(Rp1);
    for (int j = 0; j <= 3; j++)
    {
        line(img, Rp1[j], Rp1[(j + 1) % 4], Scalar(255, 0, 255), 2);
    }

    // Calculate the appropriate rectangle line
    GetRecLine(Rp1);
    line(img, Rp1[0], Rp1[1], Scalar(255, 255, 0), 5);

    Mat Find2;
    cvtColor(Rec2, Rec2, CV_BGR2GRAY);
    threshold(Rec2, Find2, 0, 255, THRESH_OTSU + THRESH_BINARY);
    findContours(Find2, tf_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    // 再次查找最大矩形
    int max_area1 = 0;
    int index1;
    for (int i = 0; i < tf_contours.size(); i++)
    {
        double tmparea = fabs(contourArea(tf_contours[i]));
        if (tmparea > max_area1)
        {
            index1 = i;
            max_area1 = tmparea;
        }
    }
    contours.pop_back(); // 只存储一个最大矩形
    contours.push_back(tf_contours[index1]);
    // drawContours(img, contours, -1, Scalar(255,0,255), 6);
    // Get the rectangle four points
    RotatedRect R1 = minAreaRect(contours.back());
    Point2f Rp[4];
    R1.points(Rp);
    for (int j = 0; j <= 3; j++)
    {
        line(img, Rp[j], Rp[(j + 1) % 4], Scalar(255, 180, 0), 2);
    }
    // Calculate the appropriate rectangle line
    GetRecLine(Rp);
    line(img, Rp[0], Rp[1], Scalar(255, 255, 0), 5);

    // Right straight line
    dst[0].x = pot[b].x;
    dst[0].y = pot[b].y;
    dst[1].x = pot[b].x;
    dst[1].y = pot[b].y + 200;

    // Bottom line
    float bottom = pot[b1].y - Rp[1].y; // Straight line and bottom vertex distance
    dst[2].x = Rp[0].x;
    dst[2].y = Rp[0].y + bottom + 10;
    dst[3].x = Rp[1].x;
    dst[3].y = Rp[1].y + bottom + 10;
    // Left straight line
    dst[4].x = pot[mib].x;
    dst[4].y = pot[mib].y;
    dst[5].x = pot[mib].x;
    dst[5].y = pot[mib].y + 200;

    // Top straight line
    float top = pot[mib1].y - Rp1[1].y;
    dst[6].x = Rp1[0].x;
    dst[6].y = Rp1[0].y + top - 10;
    dst[7].x = Rp1[1].x;
    dst[7].y = Rp1[1].y + top - 10;
}

Mat Preprocess(const Mat &gray)
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

    return dilate2;
}

void GetCrossPoint(Point2f P1[], Point2f P2[], Point2f dst[])
{
    /*
    equation：x = (b0*c1 - b1*c0)/D
              y = (a1*c0 - a0*c1)/D 
     */
    // a1
    float a1 = P1[0].y - P1[1].y;
    // b1
    float b1 = P1[1].x - P1[0].x;
    // c1
    float c1 = P1[0].x * P1[1].y - P1[1].x * P1[0].y;

    // a2
    float a2 = P2[0].y - P2[1].y;
    // b2
    float b2 = P2[1].x - P2[0].x;
    // c2
    float c2 = P2[0].x * P2[1].y - P2[1].x * P2[0].y;
    // D
    float D = a1 * b2 - a2 * b1;
    dst[knum].x = (b1 * c2 - b2 * c1) / D;
    dst[knum].y = (a2 * c1 - a1 * c2) / D;
    knum++;
}

void SortCorners(std::vector<cv::Point2f> &corners, cv::Point2f center)
{
    std::vector<cv::Point2f> top, bot; // Store temporary points
    vector<Point2f> backup = corners;  // Output

    sort(corners.begin(), corners.end(), x_sort);

    for (int i = 0; i < corners.size(); i++)
    {
        if (corners[i].y < center.y && top.size() < 2)
            top.push_back(corners[i]);
        else
            bot.push_back(corners[i]);
    }
    corners.clear();

    if (top.size() == 2 && bot.size() == 2)
    {
        cv::Point2f tl = top[0].x > top[1].x ? top[1] : top[0];
        cv::Point2f tr = top[0].x > top[1].x ? top[0] : top[1];
        cv::Point2f bl = bot[0].x > bot[1].x ? bot[1] : bot[0];
        cv::Point2f br = bot[0].x > bot[1].x ? bot[0] : bot[1];

        corners.push_back(tl);
        corners.push_back(tr);
        corners.push_back(br);
        corners.push_back(bl);
    }
    else
    {
        corners = backup;
    }
}

void CalcDstSize(const vector<cv::Point2f> &corners)
{
    int h1 = sqrt((corners[0].x - corners[3].x) * (corners[0].x - corners[3].x) +
                  (corners[0].y - corners[3].y) * (corners[0].y - corners[3].y));
    int h2 = sqrt((corners[1].x - corners[2].x) * (corners[1].x - corners[2].x) +
                  (corners[1].y - corners[2].y) * (corners[1].y - corners[2].y));
    kdst_hight = MAX(h1, h2);

    int w1 = sqrt((corners[0].x - corners[1].x) * (corners[0].x - corners[1].x) +
                  (corners[0].y - corners[1].y) * (corners[0].y - corners[1].y));
    int w2 = sqrt((corners[2].x - corners[3].x) * (corners[2].x - corners[3].x) +
                  (corners[2].y - corners[3].y) * (corners[2].y - corners[3].y));
    kdst_width = MAX(w1, w2);
}

void GetRecLine(Point2f dst[])
{
    // The X-axis changes.
    double d1 = fabs(dst[0].y - dst[1].y);
    double d2 = fabs(dst[0].y - dst[2].y);
    double d3 = fabs(dst[0].y - dst[3].y);
    double minty = (d1 < d2) ? (d1 < d3 ? d1 : d3) : (d2 < d3 ? d2 : d3);

    if (d1 == minty)
    {
        return ;
    }
    else if (d2 == minty)
    {
        dst[1] = dst[2];
    }
    else
    {
        dst[1] = dst[3];
    }
}
