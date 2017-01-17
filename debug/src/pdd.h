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
#include "FlyCapture2.h"

#define          PDD_OSX_DEBUG       (1)   // in debug mode, we use different camera
#define    DEF_AVE_FRM_SPL_NUM       (5)   // default background reference frame number
#define  DEF_FRM_GRAB_DELAY_MS       (10)  // default frame grab delay time is milliseconds


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

struct Cam {
    
};

class Pdd {
public:
    Pdd() {initCam();};
    Pdd(const char *config);
    void setupBgRef() { bgRefFrame = grabAveFrame(); bgStatus = true; };
    void setupFgSpl() { fgSplFrame = grabAveFrame(); fgStatus = true; };
    void applyDiff();
    void update();
    void resetOptions();
    void loadOptions(const char * config);
    void showFrameInfo();
    void showBg(){ if(!bgRefFrame.empty()) cv::imshow("preview", bgRefFrame); };
    void showRaw(){ if(!rawFrame.empty()) cv::imshow("preview", rawFrame); };
    void showFg(){ if(!fgSplFrame.empty()) cv::imshow("preview", fgSplFrame); };
    void showDiff() { if(!targetFrame.empty()) cv::imshow("preview", targetFrame); };
private:
    unsigned int parseOption(const std::string & name, unsigned int def_value);
    void initCam();
    bool grabRawFrame();
    cv::Mat grabAveFrame();
    bool grayOnly = true;
#if PDD_OSX_DEBUG
    cv::VideoCapture cam;
#else
    FlyCapture2::Camera camera;
#endif
    cv::Mat rawFrame;
    cv::Mat bgRefFrame;
    cv::Mat fgSplFrame;
    cv::Mat targetFrame;
    std::map<std::string, std::string> options;
    
    int frameType = CV_8UC3;
    int frameChannel = 3;
    cv::Size frameSize;
    
    bool camStatus = false;
    bool bgStatus = false;
    bool fgStatus = false;
    
    
};



#endif /* pdd_h */