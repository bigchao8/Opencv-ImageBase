#include "imagebase.hpp"

using namespace std;
using namespace cv;
namespace spd = spdlog;
int debug = false;

int main(int argc, char **argv)
{
    struct timeval start, finish;
    gettimeofday(&start, NULL);
    double totaltime = 0.0;

    int mdhandle;
    char text[1024];
    Mat src;
    
    if(argv[3])
    if (0 == strncmp(argv[3], "--debug", 7))
    {
        cout<< " --debug start "<<endl;
        debug = true;
    }

    src = imread(argv[1], IMREAD_COLOR);
    if (src.empty())
    {
        cout << "Please check the filename when loading the image." << endl;
        exit(1);
    }
    if (src.size().height < 36 || src.size().width < 3)
    {
        ipinfo->error("error src size w: {}  h: {}", src.size().width, src.size().height);
        exit(-1);
    }

    //Read the exif
    Improcessing *pa = new Fjpg();
    mdhandle = pa->Exif(argv[1]);
    delete pa;
    // Mobile
    if (MOBILE == mdhandle)
    {

        ipinfo->info("Mobile-------");
        // moire judge
        pa = new Interarea;
        double moire = pa->Moire(src);
        delete pa;
        ipinfo->info("The largest rectangular area : {}", moire);

        if (moire > 25000.0)
        {
            ipinfo->warn(" Call the Mole treatment!");
            ipinfo->info("Mole area : {}", moire);
            pa = new Median;
            pa->NoiseReduction(src);
            delete pa;
            ipinfo->info("Mdian successful");

            pa = new Bilateral;
            pa->NoiseReduction(src);
            delete pa;
            ipinfo->info("bilateral successful!");

            pa = new Smooth;
            pa->NoiseReduction(src);
            delete pa;
            ipinfo->info("Smooth successful!");

            pa = new Projection;
            pa->Affine(src, COMPUTER_SCREEN);
            delete pa;
            ipinfo->info("Affine successful");

            pa = new Adaptive;
            pa->Binarization(src);
            delete pa;
            ipinfo->info("Adaptive successful");

            pa = new Opening;
            pa->Rlines(src);
            delete pa;

            if(argv[3])
            if (0 == strncmp(argv[3], "--resizeout", 11))
            {
                ipinfo->warn("The picture will resize.");
                resize(src, src, Size(2560, src.size().height * (2560 / src.size().width)));
                ipinfo->info("Size after adjustment. width:{} height : {}", src.size().width, src.size().height);
            }
            if (!argv[2])
            {
                imwrite("results.jpg", src);
            }
            else
            {
                imwrite(argv[2], src);
            }
        }
        else
        {
            pa = new Saturation;
            if (!pa->Edgedetection(src))
            {
                delete pa;
                pa = new Projection;
                pa->Affine(src, MOBILE);
            }
            delete pa;
            ipinfo->info("Edgedetection successful");

            pa = new Fourier;
            pa->TiltCorrection(src);
            delete pa;

            pa = new Sharpen;
            pa->Fuzzyre(src);
            delete pa;

            pa = new Adaptive;
            pa->Binarization(src);
            delete pa;
            ipinfo->info("Adaptive successful");

            pa = new Opening;
            pa->Rlines(src);
            delete pa;

            ipinfo->info("Rlines successful");
            if(argv[3])
            if (0 == strncmp(argv[3], "--resizeout", 11))
            {
                ipinfo->warn("The picture will resize.");
                resize(src, src, Size(2560, src.size().height * (2560 / src.size().width)));
                ipinfo->info("Size after adjustment. width:{} height : {}", src.size().width, src.size().height);
            }
            if (!argv[2])
            {
                imwrite("results.jpg", src);
            }
            else
            {
                imwrite(argv[2], src);
            }
        }
    }
    if (SCANNER == mdhandle)
    {
        ipinfo->info("Scanner or computer-------");

        pa = new Projection;
        pa->Affine(src, SCANNER);
        delete pa;
        ipinfo->info("Affine successful");

        pa = new Opening;
        pa->Rlines(src);
        delete pa;
        if(argv[3])
        if (0 == strncmp(argv[3], "--resizeout", 11))
        {
            ipinfo->warn("The picture will resize.");
            resize(src, src, Size(2560, src.size().height * (2560 / src.size().width)));
            ipinfo->info("Size after adjustment. width:{} height : {}", src.size().width, src.size().height);
        }
        cvtColor(src, src, CV_RGB2GRAY);
        if (!argv[2])
        {
            imwrite("results.jpg", src);
        }
        else
        {
            imwrite(argv[2], src);
        }
    }

    if (SCREENSHOTS == mdhandle)
    {
        cvtColor(src, src, CV_RGB2GRAY);
        if (!argv[2])
        {
            imwrite("results.jpg", src);
        }
        else
        {
            imwrite(argv[2], src);
        }
    }

    gettimeofday(&finish, NULL);
    totaltime = finish.tv_sec - start.tv_sec + (finish.tv_usec - start.tv_usec) / 1000000.0;

    cout << "This program works:" << totaltime << "s！" << endl;

    return 0;
}