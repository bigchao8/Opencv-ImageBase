#include "imagebase.hpp"

namespace spd = spdlog;

std::shared_ptr<spdlog::logger> ipinfo;
typedef unsigned char uchar;
short jpegxdensity;
short jpegydensity;
typedef struct
{
    uchar *Data;
    int Type;
    unsigned Size;
} Section_t;

typedef struct
{
    char FileName[PATH_MAX + 1];
    time_t FileDateTime;
    struct
    {
        // Info in the jfif header.
        // This info is not used much - jhead used to just replace it with default
        // values, and over 10 years, only two people pointed this out.
        char Present;
        char ResolutionUnits;
        short XDensity;
        short YDensity;
    } JfifHeader;

    unsigned FileSize;
    char CameraMake[32];
    char CameraModel[40];
    char DateTime[20];
    unsigned Height, Width;
    int Orientation;
    int IsColor;
    int Process;
    int FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    float CCDWidth;
    float ExposureBias;
    float DigitalZoomRatio;
    int FocalLength35mmEquiv; // Exif 2.2 tag - usually not present.
    int Whitebalance;
    int MeteringMode;
    int ExposureProgram;
    int ExposureMode;
    int ISOequivalent;
    int LightSource;
    int DistanceRange;

    float xResolution;
    float yResolution;
    int ResolutionUnit;

    char Comments[MAX_COMMENT_SIZE];
    int CommentWidthchars; // If nonzero, widechar comment, indicates number of chars.

    unsigned ThumbnailOffset;   // Exif offset to thumbnail
    unsigned ThumbnailSize;     // Size of thumbnail.
    unsigned LargestExifOffset; // Last exif data referenced (to check if thumbnail is at end)

    char ThumbnailAtEnd; // Exif header ends with the thumbnail
                         // (we can only modify the thumbnail if its at the end)
    int ThumbnailSizeOffset;

    int DateTimeOffsets[MAX_DATE_COPIES];
    int numDateTimeTags;

    int GpsInfoPresent;
    char GpsLat[31];
    char GpsLong[31];
    char GpsAlt[20];

    int QualityGuess;
} ImageInfo_t;

ImageInfo_t ImageInfo;

// jpgfile.c functions
typedef enum
{
    READ_METADATA = 1,
    READ_IMAGE = 2,
    READ_ALL = 3,
    READ_ANY = 5 // Don't abort on non-jpeg files.
} ReadMode_t;

static Section_t *Sections = NULL;
static int SectionsAllocated;
static int SectionsRead;
static int HaveAll;
int ShowTags = FALSE;     // Do not show raw by default.
static int Quiet = FALSE; // Be quiet on success (like unix programs)

//--------------------------------------------------------------------------
// Check sections array to see if it needs to be increased in size.
//--------------------------------------------------------------------------
static void CheckSectionsAllocated(void)
{
    if (SectionsRead > SectionsAllocated)
    {
        ipinfo->info("allocation screwup");
    }
    if (SectionsRead >= SectionsAllocated)
    {
        SectionsAllocated += SectionsAllocated / 2;
        Sections = (Section_t *)realloc(Sections, sizeof(Section_t) * SectionsAllocated);
        if (Sections == NULL)
        {
            ipinfo->info("could not allocate data for entire image");
        }
    }
}
//--------------------------------------------------------------------------
// Convert a 32 bit signed value from file's native byte order
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Discard read data.
//--------------------------------------------------------------------------
void DiscardData(void)
{
    int a;
    for (a = 0; a < SectionsRead; a++)
    {
        free(Sections[a].Data);
    }
    memset(&ImageInfo, 0, sizeof(ImageInfo));
    SectionsRead = 0;
    HaveAll = 0;
}

// Parse the marker stream until SOS or EOI is seen;
//--------------------------------------------------------------------------
int ReadJpegSections(FILE *infile, ReadMode_t ReadMode)
{
    int a;
    int HaveCom = FALSE;

    a = fgetc(infile);

    if (a != 0xff || fgetc(infile) != M_SOI)
    {
        return FALSE;
    }

    ImageInfo.JfifHeader.XDensity = ImageInfo.JfifHeader.YDensity = 300;
    ImageInfo.JfifHeader.ResolutionUnits = 1;

    int itemlen;
    int prev;
    int marker = 0;
    int ll, lh, got;
    uchar *Data;

    CheckSectionsAllocated();
    prev = 0;
    for (a = 0;; a++)
    {
        marker = fgetc(infile);
        if (marker != 0xff && prev == 0xff)
            break;
        if (marker == EOF)
        {
            ipinfo->info("Unexpected end of file");
        }
        prev = marker;
    }

    if (a > 10)
    {
        ipinfo->info("Extraneous {} padding bytes before section {}", a - 1, marker);
    }

    Sections[SectionsRead].Type = marker;

    // Read the length of the section.
    lh = fgetc(infile);
    ll = fgetc(infile);
    if (lh == EOF || ll == EOF)
    {
        ipinfo->info("Unexpected end of file");
    }

    itemlen = (lh << 8) | ll;

    if (itemlen < 2)
    {
        ipinfo->info("invalid marker");
    }

    Sections[SectionsRead].Size = itemlen;

    Data = (uchar *)malloc(itemlen);
    if (Data == NULL)
    {
        ipinfo->warn("Could not allocate memory");
    }
    Sections[SectionsRead].Data = Data;

    // Store first two pre-read bytes.
    Data[0] = (uchar)lh;
    Data[1] = (uchar)ll;

    got = fread(Data + 2, 1, itemlen - 2, infile); // Read the whole section.
    if (got != itemlen - 2)
    {
        ipinfo->warn("Premature end of file?");
    }
    SectionsRead += 1;
    switch (marker)
    {
    case M_JFIF:
        // Regular jpegs always have this tag, exif images have the exif
        // marker instead, althogh ACDsee will write images with both markers.
        // this program will re-create this marker on absence of exif marker.
        // hence no need to keep the copy from the file.
        if (memcmp(Data + 2, "JFIF\0", 5))
        {
            ipinfo->info("Header missing JFIF marker");
        }
        if (itemlen < 16)
        {
            ipinfo->info("Jfif header too short");
            goto ignore;
        }
        ImageInfo.JfifHeader.Present = TRUE;
        ImageInfo.JfifHeader.ResolutionUnits = Data[9];
        ImageInfo.JfifHeader.XDensity = (Data[10] << 8) | Data[11];
        ImageInfo.JfifHeader.YDensity = (Data[12] << 8) | Data[13];
        if (ShowTags)
        {
            ipinfo->info("JFIF SOI marker: Units: {}", ImageInfo.JfifHeader.ResolutionUnits);
            switch (ImageInfo.JfifHeader.ResolutionUnits)
            {
            case 0:
                ipinfo->info("(aspect ratio)");
                break;
            case 1:
                ipinfo->info("(dots per inch)");
                break;
            case 2:
                ipinfo->info("(dots per cm)");
                break;
            default:
                ipinfo->info("(unknown)");
                break;
            }
            ipinfo->info("  X-density={} Y-density={}", ImageInfo.JfifHeader.XDensity, ImageInfo.JfifHeader.YDensity);
            jpegxdensity = ImageInfo.JfifHeader.XDensity;
            jpegydensity = ImageInfo.JfifHeader.YDensity;
            if (Data[14] || Data[15])
            {
                ipinfo->info("Ignoring jfif header thumbnail\n");
            }
        }

    ignore:

        free(Sections[--SectionsRead].Data);
        break;
    }

    return TRUE;
}

//--------------------------------------------------------------------------
// Read image data.
//--------------------------------------------------------------------------
int ReadJpegFile(const char *FileName, ReadMode_t ReadMode)
{

    FILE *infile;
    int ret;
    infile = fopen(FileName, "rb"); // Unix ignores 'b', windows needs it.
    if (infile == NULL)
    {
        ipinfo->info("can't open {}", FileName);
        return FALSE;
    }
    // Scan the JPEG headers.
    ret = ReadJpegSections(infile, ReadMode);
    if (!ret)
    {
        if (ReadMode == READ_ANY)
        {
            // Process any files mode.  Ignore the fact that it's not
            // a jpeg file.
            ret = TRUE;
        }
        else
        {
            ipinfo->info("Not JPEG: {}", FileName);
        }
    }
    fclose(infile);
    if (ret == FALSE)
    {
        DiscardData();
    }
    return ret;
}
int Fjpg::Exif(char *path)
{
    ExifEntry_t eq;
    ExifEntry_t xr;
    ExifEntry_t yr;
    ExifEntry_t ori;
    char text[1024];
    bool par;
    // Access path
    std::string filename = path;

    // Read the binary stream.
    std::ifstream stream(filename.c_str(), std::ios_base::in | std::ios_base::binary);

    // Get exif
    ExifReader rex(stream);
    // read parse
    par = rex.parse();

    eq = rex.getTag(MAKE);
    xr = rex.getTag(XRESOLUTION);
    yr = rex.getTag(YRESOLUTION);
    ori = rex.getTag(ORIENTATION);

    // Create the Logs folder permissions for all users.
    mkdir("logs", S_IRWXG | S_IRWXO | S_IRWXU);

    // Srand
    srand(time(NULL));
    int n = rand() % 10000;
    sprintf(text, "%s/%d.txt", "logs", n);

    // Create a daily logger - a new file is created every day on 2:30am
    ipinfo = spd::daily_logger_mt("daily_logger", text, 2, 30);

    // trigger flush if the log severity is error or higher
    ipinfo->flush_on(spd::level::err);
    ipinfo->debug(filename);

    if (par)
    {
        ipinfo->info("read exif successful");
        ipinfo->warn("parse:{}", par);
        if (yr.field_u_rational.size() == 0)
        {
            ReadMode_t ReadMode;
            ReadMode = READ_METADATA;
            ShowTags = TRUE;
            ReadJpegFile(path, ReadMode);
            if (jpegxdensity == 300 && jpegydensity == 300 || jpegxdensity == 600 && jpegydensity == 600)
            {
                ipinfo->info("find jpeg tfif informaion");
                return SCANNER;
            }
            else
            {
                ipinfo->info("No find image exif");
                return SCREENSHOTS;
            }
        }
        ipinfo->info("Exif YRESOLUTION : {}", yr.field_u_rational.back().first);
        if ((int)xr.field_u_rational.back().first == 300 || (int)xr.field_u_rational.back().first == 600)
        {
            ipinfo->info("Exif XRESOLUTION : {}", xr.field_u_rational.back().first);
            return SCANNER;
        }
        else
        {
            ipinfo->info("Exif XRESOLUTION : {}", xr.field_u_rational.back().first);
            return MOBILE;
        }
    }
    else
    {
        ReadMode_t ReadMode;
        ReadMode = READ_METADATA;
        ShowTags = TRUE;
        ReadJpegFile(path, ReadMode);
        if (jpegxdensity == 300 && jpegydensity == 300 || jpegxdensity == 600 && jpegydensity == 600)
        {
            ipinfo->info("find jpeg tfif informaion");
            return SCANNER;
        }
        else
        {
            ipinfo->info("No find image exif");
            return SCREENSHOTS;
        }
    }
}
