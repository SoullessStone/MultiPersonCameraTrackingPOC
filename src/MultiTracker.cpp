#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/xfeatures2d.hpp"

#include <stdio.h>
#include <stdlib.h>

// Own Stuf
#include <PlayerExtractor.h>
#include <Camera.h>
#include <Logger.h>
#include <Clock.h>
#include <TrackingModule.h>

using namespace cv;
using namespace dnn;
using namespace std;
using namespace cv::xfeatures2d;

// Roughly based on: https://github.com/opencv/opencv/blob/master/samples/dnn/object_detection.cpp
const char* keys =
"{ help  h     | | Print help message. }"
"{ model m     | | Path to a binary file of model contains trained weights. "
"It could be a file with extensions .caffemodel (Caffe), "
".pb (TensorFlow), .t7 or .net (Torch), .weights (Darknet) }"
"{ config c    | | Path to a text file of model contains network configuration. "
"It could be a file with extensions .prototxt (Caffe), .pbtxt (TensorFlow), .cfg (Darknet) }"
"{ framework f | | Optional name of an origin framework of the model. Detect it automatically if it does not set. }"
"{ classes     | | Optional path to a text file with names of classes to label detected objects. }"
"{ mean        | | Preprocess input image by subtracting mean values. Mean values should be in BGR order and delimited by spaces. }"
"{ scale       |  1 | Preprocess input image by multiplying on a scale factor. }"
"{ width       | -1 | Preprocess input image by resizing to a specific width. }"
"{ height      | -1 | Preprocess input image by resizing to a specific height. }"
"{ rgb         |    | Indicate that model works with RGB input images instead BGR ones. }"
"{ thr         | .5 | Confidence threshold. }"
"{ backend     |  0 | Choose one of computation backends: "
"0: default C++ backend, "
"1: Halide language (http://halide-lang.org/), "
"2: Intel's Deep Learning Inference Engine (https://software.seek.intel.com/deep-learning-deployment)}"
"{ target      |  0 | Choose one of target computation devices: "
"0: CPU target (by default),"
"1: OpenCL }";

// No header file for this main-file
void initPointPairsMarcos();
void initPointPairsHudritsch();
void initPointPairsMichel();
void printList(std::vector<RecognizedPlayer>& players);

// Reference points for the three views
std::vector<PointPair> referencePointsHud;
std::vector<PointPair> referencePointsMar;
std::vector<PointPair> referencePointsMic;
// Cameras of the three views
Camera cameraHud(1, "../resources/hudritsch_short2.mp4", 0);
Camera cameraMar(2, "../resources/marcos_short2.mp4", 0);
Camera cameraMic(3, "../resources/michel_short2.mp4", 0);
// Tracking module
TrackingModule trackingModule;
// For the correction-logic
Point correctionPointA(-1,-1);
Point correctionPointB(-1,-1);
Point correctionPointC(-1,-1);
bool newCorrectionCoords = false;

// Used for correction
void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		if (correctionPointA.x == -1) {
			correctionPointA.x = x*2;
			correctionPointA.y = y*2;
		} else if (correctionPointB.x == -1) {
			correctionPointB.x = x*2;
			correctionPointB.y = y*2;
		} else {
			correctionPointC.x = x*2;
			correctionPointC.y = y*2;
		}
		newCorrectionCoords = true;
	}
}

// Correction logic
void handleCorrection(Point p, int frameId) {
	Logger::log("Correction started for point " + std::to_string(p.x) + "/" + std::to_string(p.y), 0);
	// Get Possible PlayerIds
	std::vector<int> possibleIds = trackingModule.getHistoryPlayerIds();
	int i = 1;
	Logger::log("Press Escape to abort the correction.", 1);
	for (int id : possibleIds) {
		Logger::log("Press " + std::to_string(i) + " to assign the clicked point to player #" + std::to_string(id), 1);
		i++;
	}
	int key = waitKey();
	int numberPressed = key - 48;
	// Loop until we have an input worth processing
	while (numberPressed < 1 || numberPressed > 6) {
		if (key == 27)
			break;
		Logger::log("Please click a number that is possible.", 1);
		key = waitKey();
		numberPressed = key - 48;
	}
	if (key != 27) {
		trackingModule.applyCorrection(possibleIds.at(numberPressed - 1), frameId-1, p);
	} else {
		Logger::log("Aborted Correction.", 1);
	}
}

int main(int argc, char** argv)
{
	CommandLineParser parser(argc, argv, keys);
	parser.about("Showcase of a multiperson tracking algorithm");
	if (argc == 1 || parser.has("help"))
	{
		parser.printMessage();
		return 0;
	}

	// Load classes from file
	std::vector<std::string> classes;
	if (parser.has("classes"))
	{
		std::string file = parser.get<String>("classes");
		std::ifstream ifs(file.c_str());
		if (!ifs.is_open())
			CV_Error(Error::StsError, "File " + file + " not found");
		std::string line;
		while (std::getline(ifs, line))
		{
			classes.push_back(line);
		}
	}

	// Load model and prepare darknet
	CV_Assert(parser.has("model"));
	Net net = readNet(parser.get<String>("model"), parser.get<String>("config"), parser.get<String>("framework"));
	net.setPreferableBackend(parser.get<int>("backend"));
	net.setPreferableTarget(parser.get<int>("target"));

	// initiate the reference points
	initPointPairsHudritsch();
	initPointPairsMarcos();
	initPointPairsMichel();

	// Window for correction logic
	namedWindow("Tracking", 1);
	setMouseCallback("Tracking", mouse_callback);

	// Some things to be used in the loop
	PlayerExtractor playerExtractor(classes, parser.get<float>("thr"), net, parser.get<float>("scale"), parser.get<Scalar>("mean"), parser.get<bool>("rgb"), parser.get<int>("width"), parser.get<int>("height"));
	Mat frameHud, frameMar, frameMic;
	std::vector<RecognizedPlayer> detectedPlayersHud;
	std::vector<RecognizedPlayer> detectedPlayersMar;
	std::vector<RecognizedPlayer> detectedPlayersMic;
	std::vector<Mat> outs;
	// "Forever" - We quit, when no more frames are available
	int frameId = 0;

	Clock overallClock;
	Clock iterationClock;
	Clock clock;
	overallClock.tic();
	while (true == true)
	{
		iterationClock.tic();
		clock.tic();
		frameId++;
		Logger::log("Frame #" + std::to_string(frameId), 1);

		// We provide a correction-method
		if (newCorrectionCoords)
		{
			// It is possible to change three points at a time. But two or only one is also doable
			if (correctionPointA.x != -1)
				handleCorrection(correctionPointA, frameId);
			if (correctionPointB.x != -1)
				handleCorrection(correctionPointB, frameId);
			if (correctionPointC.x != -1)
				handleCorrection(correctionPointC, frameId);
			// Reset for next correction
			correctionPointA = Point(-1,-1);
			correctionPointB = Point(-1,-1);
			correctionPointC = Point(-1,-1);
			newCorrectionCoords = false;
		}

		try
		{
			// Get new frames
			frameMar = cameraMar.getNextFrame();
			frameMic = cameraMic.getNextFrame();
			frameHud = cameraHud.getNextFrame();
		} catch (int e)
		{
			Logger::log("No frames left, show is over", 1);
			overallClock.toc("Everything took: ");
			break;
		}
		clock.toc("Load all frames: ");

		// Detect players for the three views
		outs = playerExtractor.getOuts(frameHud);
		clock.toc("Hud - Neuronal Network: ");
		detectedPlayersHud = playerExtractor.extract(frameHud, outs, referencePointsHud, 350, true);
		clock.toc("Hud - Extract Players: ");
		cv::resize(frameHud,frameHud,Size((int)(((double)frameHud.cols / (double)3)),(int)(((double)frameHud.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameHud", frameHud);
		clock.toc("Hud - Resizing and showing image: "); 

		outs = playerExtractor.getOuts(frameMar);
		clock.toc("Mar - Neuronal Network: ");
		detectedPlayersMar = playerExtractor.extract(frameMar, outs, referencePointsMar, 350, false);
		clock.toc("Mar - Extract Players: ");
		cv::resize(frameMar,frameMar,Size((int)(((double)frameMar.cols / (double)3)),(int)(((double)frameMar.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameMar", frameMar);
		clock.toc("Mar - Resizing and showing image: "); 

		outs = playerExtractor.getOuts(frameMic);
		clock.toc("Mic - Neuronal Network: ");
		detectedPlayersMic = playerExtractor.extract(frameMic, outs, referencePointsMic, 1000, true);
		clock.toc("Mic - Extract Players: ");
		cv::resize(frameMic,frameMic,Size((int)(((double)frameMic.cols / (double)3)),(int)(((double)frameMic.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameMic", frameMic);
		clock.toc("Mic - Resizing and showing image: "); 

		trackingModule.handleInput(frameId, detectedPlayersHud, detectedPlayersMar, detectedPlayersMic);
		clock.toc("Handle Input for all frames (Tracking): ");

		iterationClock.toc("************************* Sum of all works: ");
		
		//if (frameId > 500 && (frameId-1)%10 == 0)
		if ((frameId-1)%10 == 0)
		{
			trackingModule.createVideo();
			waitKey();
		}

	}
	return 0;
}

void printList(std::vector<RecognizedPlayer>& players) {
	Logger::log("/////////////////////////////////////////////", 1);
	for(RecognizedPlayer& player : players) {
		Logger::log(player.toString(), 0);
	}
}

void initPointPairsHudritsch() {
	// top line
	referencePointsHud.push_back(PointPair(1, 240, 192, 0, 0)); // 1
	referencePointsHud.push_back(PointPair(2, 386, 168, 282, 0));
	referencePointsHud.push_back(PointPair(3, 754, 118, 882, 0));
	referencePointsHud.push_back(PointPair(4, 950, 106, 1182, 0));
	referencePointsHud.push_back(PointPair(5, 1146, 108, 1482, 0)); // 5
	referencePointsHud.push_back(PointPair(6, 1554, 128, 2082, 0));
	referencePointsHud.push_back(PointPair(7, 1714, 150, 2364, 0));
	// Second line
	referencePointsHud.push_back(PointPair(8, 182, 220, 0, 140));
	referencePointsHud.push_back(PointPair(9, 342, 200, 282, 140));
	referencePointsHud.push_back(PointPair(10, 734, 148, 882, 140)); // 10
	referencePointsHud.push_back(PointPair(11, 948, 140, 1182, 140));
	referencePointsHud.push_back(PointPair(12, 1164, 136, 1482, 140));
	referencePointsHud.push_back(PointPair(13, 1592, 162, 2082, 140));
	referencePointsHud.push_back(PointPair(14, 1772, 182, 2364, 140));
	// Third line
	referencePointsHud.push_back(PointPair(15, 416, 214, 512, 284)); // 15
	referencePointsHud.push_back(PointPair(16, 708, 188, 882, 284));
	referencePointsHud.push_back(PointPair(17, 944, 174, 1182, 284));
	referencePointsHud.push_back(PointPair(18, 1188, 170, 1482, 284));
	referencePointsHud.push_back(PointPair(19, 1488, 188, 1852, 284));
	// Fourth line
	// 20 Not visible
	referencePointsHud.push_back(PointPair(21, 116, 358, 282, 590));
	referencePointsHud.push_back(PointPair(22, 1838, 322, 2082, 590)); // A
	// 23 not visible
	// Fifth line
	referencePointsHud.push_back(PointPair(24, 88, 560, 512, 896));
	referencePointsHud.push_back(PointPair(25, 494, 558, 882, 896)); // 25
	referencePointsHud.push_back(PointPair(26, 922, 548, 1182, 896));
	referencePointsHud.push_back(PointPair(27, 1372, 542, 1482, 896));
	referencePointsHud.push_back(PointPair(28, 1856, 530, 1852, 896)); // B
	// Sixth line
	// 29&30 not visible
	referencePointsHud.push_back(PointPair(31, 422, 732, 882, 1040));
	referencePointsHud.push_back(PointPair(32, 910, 750, 1182, 1040));
	referencePointsHud.push_back(PointPair(33, 1440, 732, 1482, 1040));
	// 34 - 37 not visible
	// Seventh line
	referencePointsHud.push_back(PointPair(38, 324, 968, 882, 1180));
	referencePointsHud.push_back(PointPair(39, 902, 1016, 1182, 1180));
	referencePointsHud.push_back(PointPair(40, 1532, 990, 1482, 1180)); // 40
	// 41 not visible
	// Second part Fourth line
	// 42 not visible
	referencePointsHud.push_back(PointPair(43, 288, 346, 512, 590)); // 43
	referencePointsHud.push_back(PointPair(44, 628, 310, 882, 590));
	referencePointsHud.push_back(PointPair(45, 930, 296, 1182, 590));
	referencePointsHud.push_back(PointPair(46, 1258, 294, 1482, 590));
	referencePointsHud.push_back(PointPair(47, 1636, 310, 1852, 590));
}

void initPointPairsMichel() {
	// top line
	referencePointsMic.push_back(PointPair(1, 30, 94, 0, 0)); // 1
	referencePointsMic.push_back(PointPair(2, 138, 86, 282, 0));
	referencePointsMic.push_back(PointPair(3, 400, 74, 882, 0));
	referencePointsMic.push_back(PointPair(4, 560, 72, 1182, 0));
	referencePointsMic.push_back(PointPair(5, 742, 80, 1482, 0)); // 5
	referencePointsMic.push_back(PointPair(6, 1132, 126, 2082, 0));
	referencePointsMic.push_back(PointPair(7, 1338, 164, 2364, 0));
	// Second line
	// 8 not visible
	referencePointsMic.push_back(PointPair(9, 64, 110, 282, 140));
	referencePointsMic.push_back(PointPair(10, 348, 98, 882, 140)); // 10
	referencePointsMic.push_back(PointPair(11, 516, 96, 1182, 140));
	referencePointsMic.push_back(PointPair(12, 708, 106, 1482, 140));
	referencePointsMic.push_back(PointPair(13, 1130, 162, 2082, 140));
	referencePointsMic.push_back(PointPair(14, 1356, 204, 2364, 140));
	// Third line
	referencePointsMic.push_back(PointPair(15, 80, 134, 512, 284)); // 15
	referencePointsMic.push_back(PointPair(16, 272, 130, 882, 284));
	referencePointsMic.push_back(PointPair(17, 454, 130, 1182, 284));
	referencePointsMic.push_back(PointPair(18, 666, 138, 1482, 284));
	referencePointsMic.push_back(PointPair(19, 944, 170, 1852, 284));
	// Fourth line
	// 20 & 21 not visible
	referencePointsMic.push_back(PointPair(22, 1126, 332, 2082, 590)); // A
	referencePointsMic.push_back(PointPair(23, 1450, 396, 2364, 590));
	// Fifth line
	// 24 & 25 not visible
	referencePointsMic.push_back(PointPair(26, 0, 388, 1182, 896));
	referencePointsMic.push_back(PointPair(27, 290, 436, 1482, 896));
	referencePointsMic.push_back(PointPair(28, 782, 526, 1852, 896)); // B
	// Sixth line
	// 29 - 32 not visible
	referencePointsMic.push_back(PointPair(33, 132, 596, 1482, 1040));
	referencePointsMic.push_back(PointPair(34, 1094, 790, 2082, 1040)); // C
	referencePointsMic.push_back(PointPair(35, 1590, 846, 2364, 1040));
	// Seventh line
	// 36 - 39 not visible
	referencePointsMic.push_back(PointPair(40, 0, 780, 1482, 1180)); // 40
	referencePointsMic.push_back(PointPair(41, 1074, 1052, 2082, 1180));
	referencePointsMic.push_back(PointPair(42, 1614, 1040, 2364, 1180));
	// Second part Fourth line
	// 43 not visible
	referencePointsMic.push_back(PointPair(44, 64, 220, 882, 590));
	referencePointsMic.push_back(PointPair(45, 266, 226, 1182, 590));
	referencePointsMic.push_back(PointPair(46, 518, 240, 1482, 590));
	referencePointsMic.push_back(PointPair(47, 890, 288, 1852, 590));
}

void initPointPairsMarcos() {
	// top line
	referencePointsMar.push_back(PointPair(1, 1916, 956, 0, 0)); // 1
	referencePointsMar.push_back(PointPair(2, 1500, 1068, 282, 0));
	referencePointsMar.push_back(PointPair(3, 74, 1074, 882, 0));
	// 4 - 7 not visible
	// Second line
	referencePointsMar.push_back(PointPair(8, 1806, 764, 0, 140));
	referencePointsMar.push_back(PointPair(9, 1402, 834, 282, 140));
	referencePointsMar.push_back(PointPair(10, 176, 828, 882, 140)); // 10
	// 11 - 14 not visible
	// Third line
	referencePointsMar.push_back(PointPair(15, 952, 628, 512, 284)); // 15
	referencePointsMar.push_back(PointPair(16, 284, 616, 882, 284));
	// 17 - 19 not visible
	// Fourth line
	referencePointsMar.push_back(PointPair(20, 1502, 364, 0, 590)); // 20
	referencePointsMar.push_back(PointPair(21, 1190, 336, 282, 590));
	// 22 - 23 not visible
	// Fifth line
	referencePointsMar.push_back(PointPair(24, 928, 182, 512, 896));
	referencePointsMar.push_back(PointPair(25, 592, 176, 882, 896)); // 25
	referencePointsMar.push_back(PointPair(26, 318, 186, 1182, 896));
	referencePointsMar.push_back(PointPair(27, 78, 208, 1482, 896));
	// 28 not visible
	// Sixth line
	referencePointsMar.push_back(PointPair(29, 1312, 180, 0, 1040));
	referencePointsMar.push_back(PointPair(30, 1098, 156, 282, 1040)); // 30
	referencePointsMar.push_back(PointPair(31, 628, 134, 882, 1040));
	referencePointsMar.push_back(PointPair(32, 384, 148, 1182, 1040));
	referencePointsMar.push_back(PointPair(33, 158, 168, 1482, 1040));
	// 34 & 35 not visible
	// Seventh line
	referencePointsMar.push_back(PointPair(36, 1270, 144, 0, 1180));
	referencePointsMar.push_back(PointPair(37, 1076, 118, 282, 1180));
	referencePointsMar.push_back(PointPair(38, 666, 104, 882, 1180));
	referencePointsMar.push_back(PointPair(39, 436, 112, 1182, 1180));
	referencePointsMar.push_back(PointPair(40, 234, 128, 1482, 1180)); // 40
	referencePointsMar.push_back(PointPair(41, 0, 152, 2082, 1180));
	// Second part Fourth line
	// 42 not visible
	referencePointsMar.push_back(PointPair(43, 934, 320, 512, 590)); // 43
	referencePointsMar.push_back(PointPair(44, 470, 312, 882, 590));
	referencePointsMar.push_back(PointPair(45, 122, 326, 1182, 590));
	// 46 & 47 not visible
}



/*
First Recording
void initPointPairsMarcos() {
	// 1 not visible
	referencePointsMar.push_back(PointPair(2, 214, 1066, 282, 0));
	referencePointsMar.push_back(PointPair(3, 308, 715, 882, 0));
	referencePointsMar.push_back(PointPair(4, 323, 665, 1182, 0));
	referencePointsMar.push_back(PointPair(5, 336, 633, 1482, 0)); // 5
	referencePointsMar.push_back(PointPair(6, 348, 598, 2082, 0));
	referencePointsMar.push_back(PointPair(7, 355, 586, 2364, 0));
	// Second line
	// 8 not visible
	referencePointsMar.push_back(PointPair(9, 584, 1026, 282, 140));
	referencePointsMar.push_back(PointPair(10, 440, 712, 882, 140)); // 10
	referencePointsMar.push_back(PointPair(11, 426, 664, 1182, 140));
	referencePointsMar.push_back(PointPair(12, 415, 632, 1482, 140));
	referencePointsMar.push_back(PointPair(13, 408, 598, 2082, 140));
	referencePointsMar.push_back(PointPair(14, 407, 588, 2364, 140));
	// Third line
	referencePointsMar.push_back(PointPair(15, 727, 809, 512, 284)); // 15
	referencePointsMar.push_back(PointPair(16, 596, 702, 882, 284));
	referencePointsMar.push_back(PointPair(17, 544, 658, 1182, 284));
	referencePointsMar.push_back(PointPair(18, 514, 632, 1482, 284));
	referencePointsMar.push_back(PointPair(19, 490, 607, 1852, 284));
	// Fourth line
	referencePointsMar.push_back(PointPair(20, 1732, 904, 0, 590)); // 20
	referencePointsMar.push_back(PointPair(21, 1267, 819, 282, 590));
	referencePointsMar.push_back(PointPair(22, 601, 595, 2082, 590)); // A
	referencePointsMar.push_back(PointPair(23, 582, 585, 2364, 590));
	// Fifth line
	referencePointsMar.push_back(PointPair(24, 1226, 709, 512, 896));
	referencePointsMar.push_back(PointPair(25, 1019, 664, 882, 896)); // 25
	referencePointsMar.push_back(PointPair(26, 912, 638, 1182, 896));
	referencePointsMar.push_back(PointPair(27, 833, 618, 1482, 896));
	referencePointsMar.push_back(PointPair(28, 764, 598, 1852, 896)); // B
	// Sixth line
	referencePointsMar.push_back(PointPair(29, 1796, 777, 0, 1040));
	referencePointsMar.push_back(PointPair(30, 1483, 726, 282, 1040)); // 30
	referencePointsMar.push_back(PointPair(31, 1080, 656, 882, 1040));
	referencePointsMar.push_back(PointPair(32, 967, 633, 1182, 1040));
	referencePointsMar.push_back(PointPair(33, 890, 617, 1482, 1040));
	referencePointsMar.push_back(PointPair(34, 776, 590, 2082, 1040)); // C
	referencePointsMar.push_back(PointPair(35, 748, 580, 2364, 1040));
	// Seventh line
	referencePointsMar.push_back(PointPair(36, 1804, 735, 0, 1180));
	referencePointsMar.push_back(PointPair(37, 1522, 708, 282, 1180));
	referencePointsMar.push_back(PointPair(38, 1127, 647, 882, 1180));
	referencePointsMar.push_back(PointPair(39, 1016, 627, 1182, 1180));
	referencePointsMar.push_back(PointPair(40, 938, 612, 1482, 1180)); // 40
	// Second part Fourth line
	referencePointsMar.push_back(PointPair(41, 814, 586, 2082, 1180));
	referencePointsMar.push_back(PointPair(42, 776, 576, 2364, 1180));
	referencePointsMar.push_back(PointPair(43, 1037, 749, 512, 590)); // 43
	referencePointsMar.push_back(PointPair(44, 844, 681, 882, 590));
	referencePointsMar.push_back(PointPair(45, 756, 647, 1182, 590));
	referencePointsMar.push_back(PointPair(46, 690, 623, 1482, 590));
	referencePointsMar.push_back(PointPair(47, 639, 605, 1852, 590));
}

void initPointPairsHudritsch() {
	// top line
	referencePointsHud.push_back(PointPair(1, 646, 320, 0, 0)); // 1
	referencePointsHud.push_back(PointPair(2, 674, 320, 282, 0));
	referencePointsHud.push_back(PointPair(3, 808, 329, 882, 0));
	referencePointsHud.push_back(PointPair(4, 886, 335, 1182, 0));
	referencePointsHud.push_back(PointPair(5, 983, 345, 1482, 0)); // 5
	referencePointsHud.push_back(PointPair(6, 1281, 381, 2082, 0));
	referencePointsHud.push_back(PointPair(7, 1486, 418, 2364, 0));
	// Second line
	referencePointsHud.push_back(PointPair(8, 615, 329, 0, 140));
	referencePointsHud.push_back(PointPair(9, 646, 334, 282, 140));
	referencePointsHud.push_back(PointPair(10, 762, 344, 882, 140)); // 10
	referencePointsHud.push_back(PointPair(11, 846, 351, 1182, 140));
	referencePointsHud.push_back(PointPair(12, 949, 363, 1482, 140));
	referencePointsHud.push_back(PointPair(13, 1257, 403, 2082, 140));
	referencePointsHud.push_back(PointPair(14, 1485, 444, 2364, 140));
	// Third line
	referencePointsHud.push_back(PointPair(15, 643, 350, 512, 284)); // 15
	referencePointsHud.push_back(PointPair(16, 720, 359, 882, 284));
	referencePointsHud.push_back(PointPair(17, 801, 367, 1182, 284));
	referencePointsHud.push_back(PointPair(18, 905, 381, 1482, 284));
	referencePointsHud.push_back(PointPair(19, 1078, 410, 1852, 284));
	// Fourth line
	referencePointsHud.push_back(PointPair(20, 479, 370, 0, 590)); // 20
	referencePointsHud.push_back(PointPair(21, 505, 375, 282, 590));
	referencePointsHud.push_back(PointPair(22, 1128, 531, 2082, 590)); // A
	referencePointsHud.push_back(PointPair(23, 1452, 610, 2364, 590));
	// Fifth line
	referencePointsHud.push_back(PointPair(24, 401, 426, 512, 896));
	referencePointsHud.push_back(PointPair(25, 447, 450, 882, 896)); // 25
	referencePointsHud.push_back(PointPair(26, 506, 479, 1182, 896));
	referencePointsHud.push_back(PointPair(27, 593, 522, 1482, 896));
	referencePointsHud.push_back(PointPair(28, 771, 614, 1852, 896)); // B
	// Sixth line
	referencePointsHud.push_back(PointPair(29, 299, 422, 0, 1040));
	referencePointsHud.push_back(PointPair(30, 313, 438, 282, 1040)); // 30
	referencePointsHud.push_back(PointPair(31, 369, 482, 882, 1040));
	referencePointsHud.push_back(PointPair(32, 410, 517, 1182, 1040));
	referencePointsHud.push_back(PointPair(33, 481, 575, 1482, 1040));
	referencePointsHud.push_back(PointPair(34, 824, 819, 2082, 1040)); // C
	referencePointsHud.push_back(PointPair(35, 1276, 1040, 2364, 1040));
	// Seventh line
	referencePointsHud.push_back(PointPair(36, 236, 439, 0, 1180));
	referencePointsHud.push_back(PointPair(37, 245, 461, 282, 1180));
	referencePointsHud.push_back(PointPair(38, 276, 520, 882, 1180));
	referencePointsHud.push_back(PointPair(39, 301, 566, 1182, 1180));
	referencePointsHud.push_back(PointPair(40, 351, 637, 1482, 1180)); // 40
	referencePointsHud.push_back(PointPair(41, 652, 970, 2082, 1180));
	// Second part Fourth line
	// 42 not visible
	referencePointsHud.push_back(PointPair(43, 531, 384, 512, 590)); // 43
	referencePointsHud.push_back(PointPair(44, 598, 398, 882, 590));
	referencePointsHud.push_back(PointPair(45, 673, 416, 1182, 590));
	referencePointsHud.push_back(PointPair(46, 774, 439, 1482, 590));
	referencePointsHud.push_back(PointPair(47, 960, 486, 1852, 590));
}


void initPointPairsMichel() {
	
	referencePointsMic.push_back(PointPair(1, 292, 361, 0, 0)); // 1
	referencePointsMic.push_back(PointPair(2, 457, 342, 282, 0));
	referencePointsMic.push_back(PointPair(3, 884, 303, 882, 0));
	referencePointsMic.push_back(PointPair(4, 1063, 295, 1182, 0));
	referencePointsMic.push_back(PointPair(5, 1229, 293, 1482, 0)); // 5
	referencePointsMic.push_back(PointPair(6, 1527, 296, 2082, 0));
	referencePointsMic.push_back(PointPair(7, 1618, 297, 2364, 0));
	// Second line
	referencePointsMic.push_back(PointPair(8, 263, 380, 0, 140));
	referencePointsMic.push_back(PointPair(9, 447, 367, 282, 140));
	referencePointsMic.push_back(PointPair(10, 892, 329, 882, 140)); // 10
	referencePointsMic.push_back(PointPair(11, 1087, 318, 1182, 140));
	referencePointsMic.push_back(PointPair(12, 1266, 312, 1482, 140));
	referencePointsMic.push_back(PointPair(13, 1559, 315, 2082, 140));
	referencePointsMic.push_back(PointPair(14, 1662, 315, 2364, 140));
	// Third line
	referencePointsMic.push_back(PointPair(15, 601, 390, 512, 284)); // 15
	referencePointsMic.push_back(PointPair(16, 904, 363, 882, 284));
	referencePointsMic.push_back(PointPair(17, 1121, 352, 1182, 284));
	referencePointsMic.push_back(PointPair(18, 1316, 346, 1482, 284));
	referencePointsMic.push_back(PointPair(19, 1520, 340, 1852, 284));
	// Fourth line
	referencePointsMic.push_back(PointPair(20, 5, 580, 0, 590)); // 20
	referencePointsMic.push_back(PointPair(21, 279, 555, 282, 590));
	referencePointsMic.push_back(PointPair(22, 1811, 424, 2082, 590)); // A
	referencePointsMic.push_back(PointPair(23, 1898, 419, 2364, 590));
	// Fifth line
	referencePointsMic.push_back(PointPair(24, 436, 794, 512, 896));
	referencePointsMic.push_back(PointPair(25, 1028, 728, 882, 896)); // 25
	referencePointsMic.push_back(PointPair(26, 1424, 664, 1182, 896));
	referencePointsMic.push_back(PointPair(27, 1705, 611, 1482, 896));
	referencePointsMic.push_back(PointPair(28, 1919, 561, 1852, 896)); // B
	// Sixth line
	// 29 not visible
	referencePointsMic.push_back(PointPair(30, 2, 962, 282, 1040)); // 30
	referencePointsMic.push_back(PointPair(31, 1094, 914, 882, 1040));
	referencePointsMic.push_back(PointPair(32, 1558, 808, 1182, 1040));
	referencePointsMic.push_back(PointPair(33, 1860, 716, 1482, 1040));
	// 34 not visible
	// 35 not visible
	// Seventh line
	// 36 not visible
	// 37 not visible
	//referencePointsMic.push_back(PointPair(37, 1622, 2, 282, 1180));
	referencePointsMic.push_back(PointPair(38, 1166, 1078, 882, 1180));
	referencePointsMic.push_back(PointPair(39, 1684, 930, 1182, 1180));
	// 40 not visible
	// Second part Fourth line
	// 41 not visible
	// 42 not visible
	referencePointsMic.push_back(PointPair(43, 536, 528, 512, 590)); // 43
	referencePointsMic.push_back(PointPair(44, 948, 490, 882, 590));
	referencePointsMic.push_back(PointPair(45, 1232, 460, 1182, 590));
	referencePointsMic.push_back(PointPair(46, 1472, 446, 1482, 590));
	referencePointsMic.push_back(PointPair(47, 1700, 428, 1852, 590));
}
*/


















