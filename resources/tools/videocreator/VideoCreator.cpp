#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "math.h"

#include <limits>
#include <numeric>
#include <iostream>
#include <vector>
#include <functional>
using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

Mat equalizeBGR(const Mat& img);

int main( int argc, char** argv )
{
	VideoWriter writer;
	int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');	// select desired codec (must be available at runtime)
	double fps = 10.0;					// framerate of the created video stream
	string filename = "./output.avi";			// name of the output video file
	writer.open(filename, codec, fps, Size(1920, 1080), true);
	// check if we succeeded
	if (!writer.isOpened()) {
		cerr << "Could not open the output video file for write\n";
	}

	for (int i = 1; i < 882; i++) {
		Mat tracking = imread("input/" + std::to_string(i) + "_tracking.jpg");
		Mat frameHud = imread("input/" + std::to_string(i) + "_frameHud.jpg");
		Mat frameMar = imread("input/" + std::to_string(i) + "_frameMar.jpg");
		Mat frameMic = imread("input/" + std::to_string(i) + "_frameMic.jpg");
		Mat filler(215, 1250, CV_8UC3, Scalar(0,0,0));
		Mat fillerSide(1080, 30, CV_8UC3, Scalar(0,0,0));
		// Korrektur für dunkles Bild
		frameMar = frameMar * 1.8;

		// Neues Frame zusammenbasteln
		Mat frame(1080, 1920, tracking.type());

		cv::Mat roi = frame(cv::Rect(0,0,640,360));
		frameHud.copyTo(roi);
		roi = frame(cv::Rect(0,360,640,360));
		frameMar.copyTo(roi);
		roi = frame(cv::Rect(0,720,640,360));
		frameMic.copyTo(roi);
		roi = frame(cv::Rect(640,215,1250,650));
		tracking.copyTo(roi);
		roi = frame(cv::Rect(640,0,1250,215));
		filler.copyTo(roi);
		roi = frame(cv::Rect(640,865,1250,215));
		filler.copyTo(roi);
		roi = frame(cv::Rect(1890,0,30,1080));
		fillerSide.copyTo(roi);

		writer.write(frame);

/*
		cv::resize(frame,frame,Size((int)(((double)frame.cols / (double)3)),(int)(((double)frame.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("tracking", tracking);
		imshow("frameHud", frameHud);
		imshow("frameMar", frameMar);
		imshow("frameMic", frameMic);
		imshow("frame", frame);
		waitKey();
*/
		
	}
}
