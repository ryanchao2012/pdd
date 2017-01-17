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
#include <opencv2/video/background_segm.hpp>


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

Pdd::Pdd(const char * config) {
    initCam();
    loadOptions(config);
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
    FlyCapture2::Camera camera;
    FlyCapture2::CameraInfo camInfo;
    error = camera.Connect( 0 );
    if (error != FlyCapture2::PGRERROR_OK)
    {
        std::cout << "Failed to connect to camera" << std::endl;
        return;
    }
    
    // Get the camera info and print it out
    error = camera.GetCameraInfo( &camInfo );
    if ( error != PGRERROR_OK )
    {
        std::cout << "Failed to get camera info from camera" << std::endl;
        return;
    }
    std::cout << camInfo.vendorName << " " << camInfo.modelName << " " << camInfo.serialNumber << std::endl;

    error = camera.StartCapture();
    if ( error == PGRERROR_ISOCH_BANDWIDTH_EXCEEDED )
    {
        std::cout << "Bandwidth exceeded" << std::endl;
        return;
    }
    else if ( error != PGRERROR_OK )
    {
        std::cout << "Failed to start image capture" << std::endl;
        return;
    }
    camStatus = true;
#endif
}

void Pdd::loadOptions(const char * config) {
    std::istringstream is_file(config);
    std::string line;
    while( std::getline(is_file, line) )
    {
        std::istringstream is_line(line);
        std::string key;
        if( std::getline(is_line, key, '=') )
        {
            std::string value;
            if( std::getline(is_line, value) ) { options[key] = value; }
        }
    }
}


void Pdd::resetOptions() {
    options.clear();
    options["numAveFrmSpl"] = std::to_string(DEF_AVE_FRM_SPL_NUM);
    options["msFrmGrabDelay"] = std::to_string(DEF_FRM_GRAB_DELAY_MS);
}

void Pdd::update() {
    if(grabRawFrame()) {
        imshow("preview", rawFrame);
    } else {
        std::cout << "!\n";
    }
}


void Pdd::applyDiff() {
    if(bgStatus && fgStatus) {
        targetFrame = cv::Mat::zeros(frameSize, frameType);
        Ptr<BackgroundSubtractor> pMOG2 = cv::createBackgroundSubtractorMOG2(100, 16, 0);
        cv::Mat fgMask(frameSize, frameType),
                temp(frameSize, frameType),
                noise = cv::Mat::zeros(frameSize, CV_32FC(frameChannel)),
                bg;
        for(int i = 0; i < 100; i++) {
            bgRefFrame.convertTo(bg, CV_32FC(frameChannel));
            cv::randn(noise, 0.0, 5.0);
            bg += noise;
            bg.convertTo(temp, frameType);
            pMOG2->apply(temp, fgMask);
        }
        
        pMOG2->apply(fgSplFrame, fgMask, 0);
        fgSplFrame.copyTo(targetFrame, fgMask);
        
    }
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
    FlyCapture2::Error error = camera.RetrieveBuffer( &rawImage );
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
    
    std::cout << "Target Frame: " << targetFrame.cols << " x " << targetFrame.rows << ", type: " << targetFrame.type() << ", empty: " << targetFrame.empty() << std::endl;

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

