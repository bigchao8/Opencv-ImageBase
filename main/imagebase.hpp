#pragma onece
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
#include <sys/time.h>
#include "spdlog/spdlog.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo.hpp>


// The global variable
extern int debug;

#define M_SOF0 0xC0 // Start Of Frame N
#define M_SOF1 0xC1 // N indicates which compression process
#define M_SOF2 0xC2 // Only SOF0-SOF2 are now in common use
#define M_SOF3 0xC3
#define M_SOF5 0xC5 // NB: codes C4 and CC are NOT SOF markers
#define M_SOF6 0xC6
#define M_SOF7 0xC7
#define M_SOF9 0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI 0xD8   // Start Of Image (beginning of datastream)
#define M_EOI 0xD9   // End Of Image (end of datastream)
#define M_SOS 0xDA   // Start Of Scan (begins compressed data)
#define M_JFIF 0xE0  // Jfif marker
#define M_EXIF 0xE1  // Exif marker.  Also used for XMP data!
#define M_XMP 0x10E1 // Not a real tag (same value in file as Exif!)
#define M_COM 0xFE   // COMment
#define M_DQT 0xDB   // Define Quantization Table
#define M_DHT 0xC4   // Define Huffmann Table
#define M_DRI 0xDD
#define M_IPTC 0xED // IPTC marker

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define MAX_COMMENT_SIZE 16000
#define MAX_DATE_COPIES 10
#define PSEUDO_IMAGE_MARKER 0x123; // Extra value.

/**
 * @brief Jpeg markers that can encounter in Jpeg file
 */
enum AppMarkerTypes
{
    SOI   = 0xD8, SOF0  = 0xC0, SOF2  = 0xC2, DHT   = 0xC4,
    DQT   = 0xDB, DRI   = 0xDD, SOS   = 0xDA,

    RST0  = 0xD0, RST1  = 0xD1, RST2  = 0xD2, RST3  = 0xD3,
    RST4  = 0xD4, RST5  = 0xD5, RST6  = 0xD6, RST7  = 0xD7,

    APP0  = 0xE0, APP1  = 0xE1, APP2  = 0xE2, APP3  = 0xE3,
    APP4  = 0xE4, APP5  = 0xE5, APP6  = 0xE6, APP7  = 0xE7,
    APP8  = 0xE8, APP9  = 0xE9, APP10 = 0xEA, APP11 = 0xEB,
    APP12 = 0xEC, APP13 = 0xED, APP14 = 0xEE, APP15 = 0xEF,

    COM   = 0xFE, EOI   = 0xD9
};

/**
 * @brief Base Exif tags used by IFD0 (main image)
 */
enum ExifTagName
{
    IMAGE_DESCRIPTION       = 0x010E,   ///< Image Description: ASCII string
    MAKE                    = 0x010F,   ///< Description of manufacturer: ASCII string
    MODEL                   = 0x0110,   ///< Description of camera model: ASCII string
    ORIENTATION             = 0x0112,   ///< Orientation of the image: unsigned short
    XRESOLUTION             = 0x011A,   ///< Resolution of the image across X axis: unsigned rational
    YRESOLUTION             = 0x011B,   ///< Resolution of the image across Y axis: unsigned rational
    RESOLUTION_UNIT         = 0x0128,   ///< Resolution units. '1' no-unit, '2' inch, '3' centimeter
    SOFTWARE                = 0x0131,   ///< Shows firmware(internal software of digicam) version number
    DATE_TIME               = 0x0132,   ///< Date/Time of image was last modified
    WHITE_POINT             = 0x013E,   ///< Chromaticity of white point of the image
    PRIMARY_CHROMATICIES    = 0x013F,   ///< Chromaticity of the primaries of the image
    Y_CB_CR_COEFFICIENTS    = 0x0211,   ///< constant to translate an image from YCbCr to RGB format
    Y_CB_CR_POSITIONING     = 0x0213,   ///< Chroma sample point of subsampling pixel array
    REFERENCE_BLACK_WHITE   = 0x0214,   ///< Reference value of black point/white point
    COPYRIGHT               = 0x8298,   ///< Copyright information
    EXIF_OFFSET             = 0x8769,   ///< Offset to Exif Sub IFD
    INVALID_TAG             = 0xFFFF    ///< Shows that the tag was not recognized
};

/**
 * @Judge the use of equipment information.
 */
enum Equipment
{
    SCANNER = 0,
    MOBILE,
    COMPUTER_SCREEN,
    SCREENSHOTS

};

enum Endianess_t
{
    INTEL = 0x49,
    MOTO = 0x4D,
    NONE = 0x00
};

typedef std::pair<uint32_t, uint32_t> u_rational_t;
extern std::shared_ptr<spdlog::logger> ipinfo;


class Improcessing
{
  public:

    // Function: Noise processing
    // Function parameters: The mat type is passed in and out.
    virtual void NoiseReduction(cv::Mat &src) {}

    // Function: Image skew correction
    // Function parameters: The mat type is passed in and out.
    // Path must be the unprocessed source file.
    virtual void TiltCorrection(cv::Mat &src) {}

    // Function: Image z correction
    // Function parameters:The mat type is passed in and out.
    virtual void Affine(cv::Mat &src , int dv) {}

    // Function: Shadow removal
    // Function parameters:The mat type is passed in and out.
    virtual void Shadow(cv::Mat &src) {}

    // Function: Image binarization
    // Function parameters:The mat type is passed in and out.
    virtual void Binarization(cv::Mat &src) {}

    // Function: Remove the image table line.
    // Function parameters:The mat type is passed in and out.
    virtual void Rlines(cv::Mat &src) {}

    // Function: Edgedetection positioning
    // Function parameters:The mat type is passed in and out.
    virtual int Edgedetection(cv::Mat &src) {}

    // Function: Image blur detection and clear processing.
    // Function parameters:The mat type is passed in and out.
    virtual void Fuzzyre(cv::Mat &src) {}

    virtual int Exif(char* path){}

    virtual double Moire(cv::Mat &src) {}

};


class Gaussian : public Improcessing
{
  public:
    // Gauss algorithm denotes noise.
    void NoiseReduction(cv::Mat &src);
};

class Bilateral : public Improcessing
{
  public:
    // Bilateral filtering
    void NoiseReduction(cv::Mat &src);
};

class Median : public Improcessing
{
  public:
    // The median algorithm
    void NoiseReduction(cv::Mat &src);
};

class Average : public Improcessing
{
  public:
    // The mean algorithm
    void NoiseReduction(cv::Mat &src);
};

class Smooth : public Improcessing
{
    public:
    // the sommth algorithm
    void NoiseReduction(cv::Mat &src);
};


class Fourier : public Improcessing
{
  public:
    // Fourier algorithm
    void TiltCorrection(cv::Mat &src);
};

class Projection : public Improcessing
{
  public:
    // Text box algorithm.
    void Affine(cv::Mat &src , int dv);
};

class TanTriggs : public Improcessing
{
  public:
    // Coverage algorithm
    void Shadow(cv::Mat &src);
};

class FixedThreshold : public Improcessing
{
  public:
    // Fixed threshold algorithm.
    void Binarization(cv::Mat &src);
};

class OTsuThreshold : public Improcessing
{
  public:
    // OTSU algorithm
    void Binarization(cv::Mat &src);
};

class Adaptive : public Improcessing
{
  public:
    // adaptive Threashold
    void Binarization(cv::Mat &src);
};
class Floodfill : public Improcessing
{
  public:
  // floodfill 
  void Binarization(cv::Mat &src);
};

class Opening : public Improcessing
{
  public:
    // Open operation
    void Rlines(cv::Mat &src);
};

class Fjpg : public Improcessing
{
  public:
  int Exif(char* path);
};

class Interarea : public Improcessing
{
    public:
    double Moire(cv::Mat &src);
};

class Saturation : public Improcessing
{
    public:
    int Edgedetection(cv::Mat &src);
};

class Sharpen : public Improcessing
{
    public:
    void Fuzzyre(cv::Mat &src);
};

/**
 * @brief Entry which contains possible values for different exif tags
 */
struct ExifEntry_t
{
    ExifEntry_t();

    std::vector<u_rational_t> field_u_rational; ///< vector of rational fields
    std::string field_str;                      ///< any kind of textual information

    float  field_float;                         ///< Currently is not used
    double field_double;                        ///< Currently is not used

    uint32_t field_u32;                         ///< Unsigned 32-bit value
    int32_t  field_s32;                         ///< Signed 32-bit value

    uint16_t tag;                               ///< Tag number

    uint16_t field_u16;                         ///< Unsigned 16-bit value
    int16_t  field_s16;                         ///< Signed 16-bit value
    uint8_t  field_u8;                          ///< Unsigned 8-bit value
    int8_t   field_s8;                          ///< Signed 8-bit value
};

/**
 * @brief Picture orientation which may be taken from EXIF
 *      Orientation usually matters when the picture is taken by
 *      smartphone or other camera with orientation sensor support
 *      Corresponds to EXIF 2.3 Specification
 */
enum ImageOrientation
{
    IMAGE_ORIENTATION_TL = 1, ///< Horizontal (normal)
    IMAGE_ORIENTATION_TR = 2, ///< Mirrored horizontal
    IMAGE_ORIENTATION_BR = 3, ///< Rotate 180
    IMAGE_ORIENTATION_BL = 4, ///< Mirrored vertical
    IMAGE_ORIENTATION_LT = 5, ///< Mirrored horizontal & rotate 270 CW
    IMAGE_ORIENTATION_RT = 6, ///< Rotate 90 CW
    IMAGE_ORIENTATION_RB = 7, ///< Mirrored horizontal & rotate 90 CW
    IMAGE_ORIENTATION_LB = 8  ///< Rotate 270 CW
};

/**
 * @brief Reading exif information from Jpeg file
 *
 * Usage example for getting the orientation of the image:
 *
 *      @code
 *      ExifReader reader(fileName);
 *      if( reader.parse() )
 *      {
 *          int orientation = reader.getTag(Orientation).field_u16;
 *      }
 *      @endcode
 *
 */
class ExifReader
{
public:
    /**
     * @brief ExifReader constructor. Constructs an object of exif reader
     *
     * @param [in]stream An istream to look for EXIF bytes from
     */
    explicit ExifReader( std::istream& stream );
    ~ExifReader();


    /**
     * @brief Parse the file with exif info
     *
     * @return true if parsing was successful and exif information exists in JpegReader object
     */
    bool parse();

    /**
     * @brief Get tag info by tag number
     *
     * @param [in] tag The tag number
     * @return ExifEntru_t structure. Caller has to know what tag it calls in order to extract proper field from the structure ExifEntry_t
     */
    ExifEntry_t getTag( const ExifTagName tag );


private:
    std::istream& m_stream;
    std::vector<unsigned char> m_data;
    std::map<int, ExifEntry_t > m_exif;
    Endianess_t m_format;

    void parseExif();
    bool checkTagMark() const;

    size_t getFieldSize ();
    size_t getNumDirEntry() const;
    uint32_t getStartOffset() const;
    uint16_t getExifTag( const size_t offset ) const;
    uint16_t getU16( const size_t offset ) const;
    uint32_t getU32( const size_t offset ) const;
    uint16_t getOrientation( const size_t offset ) const;
    uint16_t getResolutionUnit( const size_t offset ) const;
    uint16_t getYCbCrPos( const size_t offset ) const;

    Endianess_t getFormat() const;

    ExifEntry_t parseExifEntry( const size_t offset );

    u_rational_t getURational( const size_t offset ) const;

    std::map<int, ExifEntry_t > getExif();
    std::string getString( const size_t offset ) const;
    std::vector<u_rational_t> getResolution( const size_t offset ) const;
    std::vector<u_rational_t> getWhitePoint( const size_t offset ) const;
    std::vector<u_rational_t> getPrimaryChromaticies( const size_t offset ) const;
    std::vector<u_rational_t> getYCbCrCoeffs( const size_t offset ) const;
    std::vector<u_rational_t> getRefBW( const size_t offset ) const;

private:
    static const uint16_t tagMarkRequired = 0x2A;

    //offset to the _number-of-directory-entry_ field
    static const size_t offsetNumDir = 8;

    //max size of data in tag.
    //'DDDDDDDD' contains the value of that Tag. If its size is over 4bytes,
    //'DDDDDDDD' contains the offset to data stored address.
    static const size_t maxDataSize = 4;

    //bytes per tag field
    static const size_t tiffFieldSize = 12;

    //number of primary chromaticies components
    static const size_t primaryChromaticiesComponents = 6;

    //number of YCbCr coefficients in field
    static const size_t ycbcrCoeffs = 3;

    //number of Reference Black&White components
    static const size_t refBWComponents = 6;
};
