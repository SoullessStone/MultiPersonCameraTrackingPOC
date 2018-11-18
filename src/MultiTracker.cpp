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
Camera cameraHud(1, "../resources/hudritsch_short.mp4", 140);
Camera cameraMar(2, "../resources/marcos_short.mp4", 140);
Camera cameraMic(3, "../resources/michel_short.mp4", 100);
// Tracking module
TrackingModule trackingModule;

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

	// Some things to be used in the loop
	PlayerExtractor playerExtractor(classes, parser.get<float>("thr"), net, parser.get<float>("scale"), parser.get<Scalar>("mean"), parser.get<bool>("rgb"), parser.get<int>("width"), parser.get<int>("height"));
	Mat frameHud, frameMar, frameMic;
	std::vector<RecognizedPlayer> detectedPlayersHud;
	std::vector<RecognizedPlayer> detectedPlayersMar;
	std::vector<RecognizedPlayer> detectedPlayersMic;
	std::vector<Mat> outs;
	// "Forever" - We quit, when no more frames are available
	int i = 0;

	auto startTime = std::chrono::system_clock::now();

	while (true == true)
	{
		i++;
		Logger::log("Frame #" + std::to_string(i), 1);
		try
		{
			// Get new frames
			frameHud = cameraHud.getNextFrame();
			frameMar = cameraMar.getNextFrame();
			frameMic = cameraMic.getNextFrame();
		} catch (int e)
		{
			Logger::log("No frames left, show is over", 1);
			break;
		}
		
		// Detect players for the three views
		outs = playerExtractor.getOuts(frameHud);
		detectedPlayersHud = playerExtractor.extract(frameHud, outs, referencePointsHud, 350);
		//printList(detectedPlayersHud);
		cv::resize(frameHud,frameHud,Size((int)(((double)frameHud.cols / (double)3)),(int)(((double)frameHud.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameHud", frameHud);
		//waitKey();
		outs = playerExtractor.getOuts(frameMar);
		detectedPlayersMar = playerExtractor.extract(frameMar, outs, referencePointsMar, 350);
		//printList(detectedPlayersMar);
		cv::resize(frameMar,frameMar,Size((int)(((double)frameMar.cols / (double)3)),(int)(((double)frameMar.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameMar", frameMar);
		//waitKey();

		outs = playerExtractor.getOuts(frameMic);
		detectedPlayersMic = playerExtractor.extract(frameMic, outs, referencePointsMic, 1000);
		//printList(detectedPlayersMic);
		cv::resize(frameMic,frameMic,Size((int)(((double)frameMic.cols / (double)3)),(int)(((double)frameMic.rows / (double)3))), 0, 0, cv::INTER_AREA);
		imshow("frameMic", frameMic);

		trackingModule.handleInput(i, detectedPlayersHud, detectedPlayersMar, detectedPlayersMic);

		auto endTime = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = endTime - startTime;
		Logger::log("Time since start: " + std::to_string(diff.count()) + "s", 1);
		
		//waitKey();

	}
	return 0;
}

void printList(std::vector<RecognizedPlayer>& players) {
	Logger::log("/////////////////////////////////////////////", 1);
	for(RecognizedPlayer& player : players) {
		cout << player.toString() << endl;
	}
}

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
	referencePointsHud.push_back(PointPair(47, 960, 486, 2082, 590));
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
	referencePointsMic.push_back(PointPair(37, 1622, 2, 282, 1180));
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


















