//
//  pdd.h
//  pdd
//
//  Created by Tripper on 2017/1/13.
//  Copyright © 2017年 Tripper. All rights reserved.
//

#ifndef pdd_h
#define pdd_h
#include <opencv2/opencv.hpp>
#include "FlyCapture2.h"
#include <stdio.h>


#define          PDD_OSX_DEBUG       (1)   // in debug mode, we use different camera
#define    DEF_AVE_FRM_SPL_NUM       (10)  // default background reference frame number
#define  DEF_FRM_GRAB_DELAY_MS       (30)  // default frame grab delay time is milliseconds
#define           DEF_MOG2_HST       (10)  // MOG2 parameter: history
#define            DEF_MOG2_TH       (16)  // MOG2 parameter: threshold
#define    DEF_MOG2_BG_SPL_STD       (5)   // background sampling std used by MOG2

#define              DEF_BOX_X       (20)
#define              DEF_BOX_Y       (20)
#define              DEF_BOX_W       (100)
#define              DEF_BOX_H       (150)

#define         DEF_CLAHE_POW2       (5)

#define     DEF_CANNY_KNL_SIZE       (3)   //
#define       DEF_CANNY_LOW_TH       (50)  //
#define     DEF_CANNY_TH_RATIO       (3)   //


//+--------+----+----+----+----+------+------+------+------+
//|        | C1 | C2 | C3 | C4 | C(5) | C(6) | C(7) | C(8) |
//+--------+----+----+----+----+------+------+------+------+
//| CV_8U  |  0 |  8 | 16 | 24 |   32 |   40 |   48 |   56 |
//| CV_8S  |  1 |  9 | 17 | 25 |   33 |   41 |   49 |   57 |
//| CV_16U |  2 | 10 | 18 | 26 |   34 |   42 |   50 |   58 |
//| CV_16S |  3 | 11 | 19 | 27 |   35 |   43 |   51 |   59 |
//| CV_32S |  4 | 12 | 20 | 28 |   36 |   44 |   52 |   60 |
//| CV_32F |  5 | 13 | 21 | 29 |   37 |   45 |   53 |   61 |
//| CV_64F |  6 | 14 | 22 | 30 |   38 |   46 |   54 |   62 |
//+--------+----+----+----+----+------+------+------+------+

//namespace pdd {
//    class Pdd;
//}


//struct Options {
//    const std::string name = {"123"};
    
//    unsigned int numBgRefFrm = DEF_BG_REF_FRM_NUM;
//    unsigned int msFrmGrabDelay = DEF_FRM_GRAB_DELAY_MS;
//    std::map<std::string, std::string> options;
//};

class Pdd {
public:
    Pdd() {initCam();};
    void setupBgRef() { bgRefFrame = grabAveFrame(); bgStatus = true; std::cout << "BG recorded\n"; };
    void setupFgSpl() { fgSplFrame = grabAveFrame(); fgStatus = true; std::cout << "FG recorded\n"; };
    void applyMOG2();
    void applyFilter();
    void update();
    void resetOptions();
    void reloadConfig(const char * cfgFile);
    void showFrameInfo();
    void showBg(){ showFrame(bgRefFrame); };
    void showRaw(){ showFrame(rawFrame); };
    void showFg(){ showFrame(fgSplFrame); };
    void showMOG2() { showFrame(mog2Frame); };
    void showCLAHE() { showFrame(claheFrame); };
    void showCanny() { showFrame(cannyFrame); };
    void showContour() {showFrame(contourFrame); };

private:
    unsigned int parseOption(const std::string & name, unsigned int def_value);
    void initCam();
    bool grabRawFrame();
    cv::Mat grabAveFrame();
    void showFrame(cv::Mat frame){ if(!frame.empty()) cv::imwrite("preview.png", frame); };
    void applyCLAHE();
    void applyCanny();

#if PDD_OSX_DEBUG
    cv::VideoCapture cam;
#else
    FlyCapture2::Camera cam;
#endif

    cv::Mat rawFrame;
    cv::Mat bgRefFrame;
    cv::Mat fgSplFrame;
    cv::Mat mog2Frame;
    cv::Mat claheFrame;
    cv::Mat cannyFrame;
    cv::Mat contourFrame;
    std::map<std::string, std::string> options;
    
    int frameType = CV_8UC3;
    int frameChannel = 3;
    cv::Size frameSize;
    bool grayOnly = true;
    bool camStatus = false;
    bool bgStatus = false;
    bool fgStatus = false;
    bool mog2Status = false;
    bool claheStatus = false;
    bool cannyStatus = false;
    
    
};



#endif /* pdd_h */
