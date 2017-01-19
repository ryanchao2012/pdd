//
//  main.cpp
//  pdd
//
//  Created by Tripper on 2017/1/13.
//  Copyright © 2017年 Tripper. All rights reserved.
//

#include "pdd.h"
#include <iostream>

#define CFG_FILE     "../../pdd.cfg"
using namespace cv;
//using namespace std;
//const char config[] = "numBgRefFrm=5\n"
//                      "msFrmGrabDelay=10\n"
//                      "greyOnly=1";

std::map<std::string, std::string> options; // global?
Pdd pipeline;
void keyAction(char key);
int main(int argc, const char * argv[]) {
    pipeline.reloadConfig(CFG_FILE);
    char key = 0;
    
//    cv::namedWindow("preview");
    
    while (key != 'q') {
        // key = cv::waitKey(10);
        std::cin >> key;
        keyAction(key);
    }
    
    return 0;
    
}



void keyAction(char key) {
    switch (key) {
        case 'b':
            pipeline.setupBgRef();
            break;
        case 'c':
            cv::imshow("preview", Mat::zeros(1, 1, CV_8UC3));
            break;
        case 'd':
            pipeline.applyDiff();
            break;
        case 'f':
            pipeline.setupFgSpl();
            break;
        case 'r':
            pipeline.reloadConfig(CFG_FILE);
            break;
        case 'i':
            pipeline.showFrameInfo();
            break;
        case '1':
            pipeline.showRaw();
            break;
        case '2':
            pipeline.showBg();
            break;
        case '3':
            pipeline.showFg();
            break;
        case '4':
            pipeline.showDiff();
            break;
        default:
            break;
    }
}

