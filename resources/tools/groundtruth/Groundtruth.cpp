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

Mat createFieldModel();
void showNewFrames ();

struct Camera
{
	int id;
	VideoCapture cap;
	Mat frame;
	int wantedMs;
	int maxMs = 70175;
	double lastUsedMs = 0;
	int firstFrame = true;

	Camera(int inId, const std::string& inFile, int startPoint)
	{
		id = inId;
		cap = VideoCapture();
		cap.open(inFile);
		wantedMs = startPoint;
	}

	int getWantedMs() {
		return wantedMs;
	}

	double getLastUsedMs() {
		return lastUsedMs;
	}

	Mat getNextFrame()
	{
		// waiting for right frame
		for (;;) {
			int upperThreshold = (int)cap.get(CV_CAP_PROP_POS_MSEC) + 20;
			int lowerThreshold = (int)cap.get(CV_CAP_PROP_POS_MSEC) - 20;
			cap >> frame;
			if  (wantedMs < upperThreshold && wantedMs > lowerThreshold) {
				break;
			}
		}
		wantedMs += 500;
		if (frame.empty())
		{
			cout << "Reached last frame..." << endl;
		}
		cout << "Camera #" << id << ": return frame at " << cap.get(CV_CAP_PROP_POS_MSEC ) << "ms" << endl;
		lastUsedMs = cap.get(CV_CAP_PROP_POS_MSEC);
		return frame;
	}
};

struct PointPair
{
	Point p1;
	Point p2;
	int id;
	PointPair(int inId, int x1, int y1, int x2, int y2)
	{
		id = inId;
		p1 = Point(x1,y1);
		p2 = Point(x2,y2);
	}
};

Camera cameraHud(1, "../../hudritsch_short.mp4", 175);
Camera cameraMar(2, "../../marcos_short.mp4", 175);
Camera cameraMic(3, "../../michel_short.mp4", 100);
std::vector<std::string> output;
int playerId = 0;

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
    if (event == EVENT_LBUTTONDOWN)
    {
	cout << "Clicked at: " << x << ", " << y << endl;
	output.push_back(
		std::to_string(cameraHud.getWantedMs() - 500) + 
		";" + 
		std::to_string(cameraHud.getLastUsedMs()) + 
		";" + 
		std::to_string(cameraMar.getWantedMs() - 500) + 
		";" + 
		std::to_string(cameraMar.getLastUsedMs()) + 
		";" +
		std::to_string(cameraMic.getWantedMs() - 500) + 
		";" + 
		std::to_string(cameraMic.getLastUsedMs()) + 
		";" +
		std::to_string(playerId) + 
		";" +
		std::to_string(x) + 
		";" +
		std::to_string(y)
		);
	if (cameraHud.wantedMs == cameraHud.maxMs) {
		for(auto i : output) {
			cout << i << endl;
		}
		return;
	}
	showNewFrames();
    }
}

/*
 * @function main
 * @brief Main function
 */
int main( int argc, char** argv )
{
	std::string arg;
	arg = argv[1];
	if (arg == "Record")
	{		
		namedWindow("Field", 1);
		setMouseCallback("Field", mouse_callback);
		Mat fieldModel = createFieldModel();
		imshow("Field", fieldModel);
		showNewFrames();
		for (;;) {
			if ((waitKey(1) & 0xFF) == 'q') {
				for(auto i : output) {
					cout << i << endl;
				} 
				break;
			}
		}
		
	} else {
		cout << "I don't know..." << endl;
	} 
}
	


void showNewFrames () {
	Mat frameHud, frameMar, frameMic;
	frameHud = cameraHud.getNextFrame();
	cv::resize(frameHud,frameHud,Size((int)(((double)frameHud.cols / (double)3)),(int)(((double)frameHud.rows / (double)3))), 0, 0, cv::INTER_AREA);
	imshow("cameraHud", frameHud);
	frameMar = cameraMar.getNextFrame();
	cv::resize(frameMar,frameMar,Size((int)(((double)frameMar.cols / (double)3)),(int)(((double)frameMar.rows / (double)3))), 0, 0, cv::INTER_AREA);
	imshow("capMar", frameMar);
	frameMic = cameraMic.getNextFrame();
	cv::resize(frameMic,frameMic,Size((int)(((double)frameMic.cols / (double)3)),(int)(((double)frameMic.rows / (double)3))), 0, 0, cv::INTER_AREA);
	imshow("cameraMic", frameMic);

}



Mat createFieldModel() {
	// Feldgenerierung kopiert aus Hauptprojekt am 24.09.2018
	Mat field(650,1250, CV_8UC3, Scalar(153,136,119));
	// white field
	int white_x = 141;
	int white_y = 70;
	int white_width = 900;
	int white_height = 450;
	Scalar white_color(255,255,255);
	Point white_topleft(white_x,white_y) ;
	Point white_bottomleft(white_x,white_y+white_height) ;
	Point white_topright(white_x+white_width,white_y) ;
	Point white_bottomright(white_x+white_width,white_y+white_height) ;
	Point white_topthird(white_x+white_width/3,white_y) ;
	Point white_bottomthird(white_x+white_width/3,white_y+white_height) ;
	Point white_tophalf(white_x+white_width/2,white_y) ;
	Point white_bottomhalf(white_x+white_width/2,white_y+white_height) ;
	Point white_topTwoThirds(white_x+2*(white_width/3),white_y) ;
	Point white_bottomTwoThirds(white_x+2*(white_width/3),white_y+white_height) ;
	line(field, white_topleft, white_bottomleft, white_color, 2);
	line(field, white_topleft, white_topright, white_color, 2);
	line(field, white_topright, white_bottomright, white_color, 2);
	line(field, white_bottomleft, white_bottomright, white_color, 2);
	line(field, white_topthird, white_bottomthird, white_color, 2);
	line(field, white_tophalf, white_bottomhalf, white_color, 2);
	line(field, white_topTwoThirds, white_bottomTwoThirds, white_color, 2);
	// yellow field
	int yellow_x = 256;
	int yellow_y = 142;
	int yellow_width = 670;
	int yellow_height = 306;
	Scalar yellow_color(0,255,255);
	Point yellow_topleft(yellow_x,yellow_y) ;
	Point yellow_bottomleft(yellow_x,yellow_y+yellow_height) ;
	Point yellow_topright(yellow_x+yellow_width,yellow_y) ;
	Point yellow_bottomright(yellow_x+yellow_width,yellow_y+yellow_height) ;
	Point yellow_leftmiddle(yellow_x,yellow_y+yellow_height/2) ;
	Point yellow_rightmiddle(yellow_x+yellow_width,yellow_y+yellow_height/2) ;
	line(field, yellow_topleft, yellow_bottomleft, yellow_color, 2);
	line(field, yellow_topleft, yellow_topright, yellow_color, 2);
	line(field, yellow_topright, yellow_bottomright, yellow_color, 2);
	line(field, yellow_bottomleft, yellow_bottomright, yellow_color, 2);	
	line(field, yellow_leftmiddle, yellow_rightmiddle, yellow_color, 2);	

	return field;
}
