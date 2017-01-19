//
//  pdd.cpp
//  pdd
//
//  Created by Tripper on 2017/1/13.
//  Copyright © 2017年 Tripper. All rights reserved.
//

#include <stdio.h>
#include "pdd.h"
#include <thread>
#include <chrono>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <opencv2/video/background_segm.hpp>
#include "FlyCapture2.h"

using namespace cv;
using namespace std;


unsigned int Pdd::parseOption(const std::string & name, unsigned int def_value) {
    unsigned int value = 0;
    std::map<std::string, std::string>::iterator it = options.find(name);
    if (it != options.end()) {
        try {
            value = std::stoi(it->second);
        } catch(const char* message) {
            std::cout << message << std::endl;
        }
    }
    if (value == 0) {  return def_value;  }
    else {  return value;  }
}

void Pdd::initCam() {
#if PDD_OSX_DEBUG
    cam = VideoCapture(0);
    camStatus = cam.isOpened();
    if (camStatus) {
        cam.set(CV_CAP_PROP_FPS, 60);
    }
#else
    camStatus = false;
    FlyCapture2::Error error;
    FlyCapture2::CameraInfo camInfo;
    error = cam.Connect( 0 );
    if (error != FlyCapture2::PGRERROR_OK)
    {
        std::cout << "Failed to connect to camera" << std::endl;
        return;
    }
    
    // Get the camera info and print it out
    error = cam.GetCameraInfo( &camInfo );
    if ( error != FlyCapture2::PGRERROR_OK )
    {
        std::cout << "Failed to get camera info from camera" << std::endl;
        return;
    }
    std::cout << camInfo.vendorName << " " << camInfo.modelName << " " << camInfo.serialNumber << std::endl;

    error = cam.StartCapture();
    if ( error == FlyCapture2::PGRERROR_ISOCH_BANDWIDTH_EXCEEDED )
    {
        std::cout << "Bandwidth exceeded" << std::endl;
        return;
    }
    else if ( error != FlyCapture2::PGRERROR_OK )
    {
        std::cout << "Failed to start image capture" << std::endl;
        return;
    }
    camStatus = true;
#endif
}

void Pdd::reloadConfig(const char * cfgFile) {
    fstream fin;
    char line[1024];
    try {
        fin.open(cfgFile,ios::in);
        while( fin.getline(line, sizeof(line)) )
        {
            std::istringstream is_line(line);
            std::string key;
            if( std::getline(is_line, key, '=') )
            {
                std::string value;
                if( std::getline(is_line, value) ) { options[key] = value; }
                std::cout << key << " = " << value << std::endl;
            }
        }
    }
    catch(const char* message) {  std::cout << message << std::endl; }

}


void Pdd::resetOptions() {
    options.clear();
    options["numAveFrmSpl"] = std::to_string(DEF_AVE_FRM_SPL_NUM);
    options["msFrmGrabDelay"] = std::to_string(DEF_FRM_GRAB_DELAY_MS);
    options["historyMOG2"] = std::to_string(DEF_MOG2_HST);
    options["thMOG2"] = std::to_string(DEF_MOG2_TH);
    options["stdMOG2"] = std::to_string(DEF_MOG2_BG_SPL_STD);
    options["sizeKerenlCanny"] = std::to_string(DEF_CANNY_KNL_SIZE);
    options["lowThresholdCanny"] = std::to_string(DEF_CANNY_LOW_TH);
    options["ratioThresholdCanny"] = std::to_string(DEF_CANNY_TH_RATIO);
    options["pow2CLAHE"] = std::to_string(DEF_CLAHE_POW2);
}

void Pdd::update() {
    if(grabRawFrame()) {
//        imshow("preview", rawFrame);
        imwrite("preview.png", rawFrame);
    } else {
        std::cout << "!\n";
    }
}

void Pdd::applyFilter() {
    applyCLAHE();
    applyCanny();
}

void Pdd::applyCLAHE() {
    if (mog2Status) {
        std::cout << "Applying CLAHE to enhance contrast, please wait \n";
        claheFrame = cv::Mat::zeros(frameSize, frameType);
        int claheGridSize = (1 << parseOption("pow2CLAHE", DEF_CLAHE_POW2));
        Ptr<cv::CLAHE> clahe = cv::createCLAHE(1.0, Size(claheGridSize, claheGridSize));
        clahe->apply(mog2Frame, claheFrame);
        
        std::cout << "CLAHE finished!\n";
        claheStatus = true;
        
    } else { claheStatus = false; }
}

void Pdd::applyCanny() {
    if (claheStatus) {
        std::cout << "Applying Canny to find contour, please wait \n";
        cannyFrame = cv::Mat::zeros(frameSize, frameType);
        double lowThresholdCanny = (double)parseOption("lowThresholdCanny", DEF_CANNY_LOW_TH);
        double ratioThresholdCanny = (double)parseOption("ratioThresholdCanny", DEF_CANNY_TH_RATIO);
        int sizeKerenlCanny = parseOption("sizeKerenlCanny", DEF_CANNY_KNL_SIZE);
        Canny(claheFrame, cannyFrame, lowThresholdCanny, ratioThresholdCanny * lowThresholdCanny, sizeKerenlCanny);
        
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(cannyFrame, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        
        contourFrame = cv::Mat::zeros(frameSize, CV_8UC3);
        Scalar edgeColor = cv::Scalar(0, 255, 0);
        for(unsigned int i = 0; i < contours.size(); i++) {
            if(hierarchy[i][2] >= 0) {
                drawContours(contourFrame, contours, i, edgeColor, 2, 8, hierarchy, 0, Point());
                std::cout << ".";
            }
        }
        std::cout << "Canny finished!\n";
    }
}

void Pdd::applyMOG2() {
    if(bgStatus && fgStatus) {
        std::cout << "Applying MOG2 fg/bg separation, please wait \n";
        int historyMOG2 = parseOption("historyMOG2", DEF_MOG2_HST);
        double thMOG2 = (double)parseOption("thMOG2", DEF_MOG2_TH);
        double stdMOG2 = (double)parseOption("stdMOG2", DEF_MOG2_BG_SPL_STD);
        
        
        mog2Frame = cv::Mat::zeros(frameSize, frameType);
        Ptr<BackgroundSubtractor> pMOG2 = cv::createBackgroundSubtractorMOG2(historyMOG2, thMOG2, 0);
        cv::Mat fgMask(frameSize, frameType),
                temp(frameSize, frameType),
                noise = cv::Mat::zeros(frameSize, CV_32FC(frameChannel)),
                bg;
        for(int i = 0; i < historyMOG2; i++) {
            bgRefFrame.convertTo(bg, CV_32FC(frameChannel));
            cv::randn(noise, 0.0, stdMOG2);
            bg += noise;
            bg.convertTo(temp, frameType);
            pMOG2->apply(temp, fgMask);
        }
        pMOG2->apply(fgSplFrame, fgMask, 0);
        fgSplFrame.copyTo(mog2Frame, fgMask);
        std::cout << "MOG2 finished! \n";
        mog2Status = true;
    } else { mog2Status = false; }
}
bool Pdd::grabRawFrame() {
#if PDD_OSX_DEBUG
    if(!camStatus) return false;
    if (!cam.read(rawFrame)) return false;
//    Mat temp;
//    cvtColor(rawFrame, temp, CV_BGR2GRAY);
//    cropFrame = temp(Rect(obsBox.x + 1, obsBox.y + 1, obsBox.w, obsBox.h));
    
#else
    // Get the image
    FlyCapture2::Image rawImage;
    FlyCapture2::Error error = cam.RetrieveBuffer( &rawImage );
    if ( error != FlyCapture2::PGRERROR_OK )
    {
        std::cout << "capture error" << std::endl;
        return false;
    }
    
    // convert to rgb
    FlyCapture2::Image rgbImage;
    rawImage.Convert( FlyCapture2::PIXEL_FORMAT_BGR, &rgbImage );
    
    // convert to OpenCV Mat
    unsigned int rowBytes = (double)rgbImage.GetReceivedDataSize()/(double)rgbImage.GetRows();
    rawFrame = cv::Mat(rgbImage.GetRows(), rgbImage.GetCols(), CV_8UC3, rgbImage.GetData(),rowBytes);
#endif
    
    if(grayOnly) {  cvtColor(rawFrame, rawFrame, CV_BGR2GRAY);  }
    return true;
}



void Pdd::showFrameInfo() {
    std::cout << "Raw Frame: " << rawFrame.cols << " x " << rawFrame.rows << ", type: " << rawFrame.type() << ", empty: " << rawFrame.empty() << std::endl;
    std::cout << "Background Frame: " << bgRefFrame.cols << " x " << bgRefFrame.rows << ", type: " << bgRefFrame.type() << ", empty: " << bgRefFrame.empty() << std::endl;
    std::cout << "Foreground Frame: " << fgSplFrame.cols << " x " << fgSplFrame.rows << ", type: " << fgSplFrame.type() << ", empty: " << fgSplFrame.empty() << std::endl;
    
    std::cout << "MOG2 Frame: " << mog2Frame.cols << " x " << mog2Frame.rows << ", type: " << mog2Frame.type() << ", empty: " << mog2Frame.empty() << std::endl;

}


cv::Mat Pdd::grabAveFrame() {
    unsigned int numAveFrmSpl, msFrmGrabDelay;
    numAveFrmSpl = parseOption("numAveFrmSpl", DEF_AVE_FRM_SPL_NUM);
    msFrmGrabDelay = parseOption("msFrmGrabDelay", DEF_FRM_GRAB_DELAY_MS);

    grabRawFrame();
    frameChannel = rawFrame.channels();
    frameType = rawFrame.type();
    frameSize = rawFrame.size();
    
    cv::Mat aveImg = cv::Mat::zeros(rawFrame.rows, rawFrame.cols, CV_32FC(frameChannel)),
              temp = cv::Mat::zeros(rawFrame.rows, rawFrame.cols, CV_32FC(frameChannel));
    
    int _i = (int)numAveFrmSpl;
    while (_i--) {
        grabRawFrame();
        rawFrame.convertTo(temp, CV_32FC(frameChannel));
        cv::accumulate(temp, aveImg);
        std::this_thread::sleep_for(std::chrono::milliseconds(msFrmGrabDelay));
    }
    aveImg /= float(numAveFrmSpl);
    aveImg.convertTo(temp, frameType);
    return temp;
}

