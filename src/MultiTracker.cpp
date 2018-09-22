#include <fstream>
#include <sstream>
#include <iostream>

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
#include <PerspectiveToModelMapper.h>
#include <ModelImageGenerator.h>
#include <MainColorExtractor.h>
#include <NumberExtractor.h>

using namespace cv;
using namespace dnn;
using namespace std;
using namespace cv::xfeatures2d;

// Diese Datei zur Objekterkennung basiert auf: https://github.com/opencv/opencv/blob/master/samples/dnn/object_detection.cpp
const char* keys =
"{ help  h     | | Print help message. }"
"{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera.}"
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

void postprocess(Mat& frame, const std::vector<Mat>& out, Net& net);

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);

void callback(int pos, void* userdata);

std::vector<String> getOutputsNames(const Net& net);

void initPointPairsMarcos();
void initPointPairsHudritsch();

// Variables
std::vector<PointPair> allPointPairs;

float confThreshold;
std::vector<std::string> classes;

int main(int argc, char** argv)
{
	CommandLineParser parser(argc, argv, keys);
	parser.about("Use this script to run object detection deep learning networks using OpenCV.");
	if (argc == 1 || parser.has("help"))
	{
		parser.printMessage();
		return 0;
	}

	confThreshold = parser.get<float>("thr");
	float scale = parser.get<float>("scale");
	Scalar mean = parser.get<Scalar>("mean");
	bool swapRB = parser.get<bool>("rgb");
	int inpWidth = parser.get<int>("width");
	int inpHeight = parser.get<int>("height");

	// Open file with classes names.
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

	// Load a model.
	CV_Assert(parser.has("model"));
	Net net = readNet(parser.get<String>("model"), parser.get<String>("config"), parser.get<String>("framework"));
	net.setPreferableBackend(parser.get<int>("backend"));
	net.setPreferableTarget(parser.get<int>("target"));

	// Create a window
	static const std::string kWinName = "Deep learning object detection in OpenCV";
	//namedWindow(kWinName, WINDOW_NORMAL);
	int initialConf = (int)(confThreshold * 100);
	createTrackbar("Confidence threshold, %", kWinName, &initialConf, 99, callback);

	// Open a video file or an image file or a camera stream.
	VideoCapture cap;
	if (parser.has("input"))
		cap.open(parser.get<String>("input"));
	else
		cap.open(0);
	
	initPointPairsHudritsch();

	// Process frames.
	Mat frame, blob;

	int skippedFrames = 5;
	while (waitKey(1) < 0)
	{
		cap >> frame;
		if (frame.empty())
		{
			continue;
		}
		if (skippedFrames == 5) {
			skippedFrames = 0;
		} else {
			skippedFrames++;
			continue;
		}

		// Create a 4D blob from a frame.
		Size inpSize(inpWidth > 0 ? inpWidth : frame.cols,
			inpHeight > 0 ? inpHeight : frame.rows);
		blobFromImage(frame, blob, scale, inpSize, mean, swapRB, false);

		// Run a model.
		net.setInput(blob);
		if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
		{
			resize(frame, frame, inpSize);
			Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
			net.setInput(imInfo, "im_info");
		}
		std::vector<Mat> outs;
		net.forward(outs, getOutputsNames(net));

		postprocess(frame, outs, net);

		// Put efficiency information.
		std::vector<double> layersTimes;
		double freq = getTickFrequency() / 1000;
		double t = net.getPerfProfile(layersTimes) / freq;
		std::string label = format("Inference time: %.2f ms", t);
		putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));

		cv::resize(frame,frame,Size((int)(((double)frame.cols / (double)2)),(int)(((double)frame.rows / (double)2))), 0, 0, cv::INTER_AREA);
		imshow(kWinName, frame);
	}
	return 0;
}

void postprocess(Mat& frame, const std::vector<Mat>& outs, Net& net)
{
	static std::vector<int> outLayers = net.getUnconnectedOutLayers();
	static std::string outLayerType = net.getLayer(outLayers[0])->type;

	if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
	{
		cout << "im_info" << endl;
	}
	else if (outLayerType == "DetectionOutput")
	{
		cout << "DetectionOutput" << endl;
	}
	else if (outLayerType == "Region")
	{
		std::vector<int> classIds;
		std::vector<float> confidences;
		std::vector<Rect> boxes;
		int counter = 0;
		std::vector<PointPair> linesToDraw;
		std::vector<PointPair> playersToDraw;
		for (size_t i = 0; i < outs.size(); ++i)
		{
			// Network produces output blob with a shape NxC where N is a number of
			// detected objects and C is a number of classes + 4 where the first 4
			// numbers are [center_x, center_y, width, height]
			float* data = (float*)outs[i].data;
			for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
			{
				Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
				Point classIdPoint;
				double confidence;
				minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
				if (confidence > confThreshold)
				{
					int centerX = (int)(data[0] * frame.cols);
					int centerY = (int)(data[1] * frame.rows);
					int width = (int)(data[2] * frame.cols);
					int height = (int)(data[3] * frame.rows);
					int left = centerX - width / 2;
					int top = centerY - height / 2;
					int bottom = centerY + height / 2;

					// Handle Players
					if (classIdPoint.x == 0) {
						counter += 1;
						// Extract player from frame
						Mat player;
						player = frame(Rect(left, top, width, height));
						int playerNumber = counter;
						// Check if red or black player
						if (MainColorExtractor::getPlayerColor(0, 0, player) == 1) {
							playerNumber += 100;
							//cout << "Red Player" << endl;

							// Find number
							// TODO Etwas finden, das funktioniert
							/*cv::Mat greyPlayer;
							cv::cvtColor(player, greyPlayer, cv::COLOR_BGR2GRAY);
							std::array<int, 7> possibleNumbers = {1,3,4,5,6,8,9};
							for(int& i: possibleNumbers) { 
								cout << "------ Possibility for " << i << ": " << NumberExtractor::getPossibilityForPlayerAndNumber(greyPlayer, i) << endl;
								//waitKey();
							}*/
						}
						else {
							//cout << "Black Player" << endl;
						}

						// Unterster Punkt des Spielers erkennen
						Point bottomOfPlayer(centerX, bottom);
						// In der Perspektive die nähesten drei Keypoints finden
						std::array<PointPair, 3> nearestPoints = PerspectiveToModelMapper::findNearestThreePointsInModelSpace(bottomOfPlayer, allPointPairs);
						
						// bottomOfPlayer als baryzentrische Koordinaten in Bezug auf die drei nähesten Punkte beschreiben
						float u = 0.0;
						float v = 0.0;
						float w = 0.0;
						PerspectiveToModelMapper::barycentric(bottomOfPlayer, nearestPoints[0].p1, nearestPoints[1].p1, nearestPoints[2].p1, u, v, w);
						//circle(frame, Point(nearestPoints[0].p1.x, nearestPoints[0].p1.y), 10, Scalar(0, 0, 255));
						//circle(frame, Point(nearestPoints[1].p1.x, nearestPoints[1].p1.y), 10, Scalar(0, 0, 255));
						//circle(frame, Point(nearestPoints[2].p1.x, nearestPoints[2].p1.y), 10, Scalar(0, 0, 255));
						// Position des Spielers in Modelkoordinaten ausdrücken
						float x_part = u*(float)nearestPoints[0].p2.x + v*(float)nearestPoints[1].p2.x + w*(float)nearestPoints[2].p2.x;
						float y_part = u*(float)nearestPoints[0].p2.y + v*(float)nearestPoints[1].p2.y + w*(float)nearestPoints[2].p2.y;

						if (x_part < 0) {
							x_part = 0;
						}
						if (y_part < 0) {
							y_part = 0;
						}

						PointPair playerPointPair(playerNumber, bottomOfPlayer.x, bottomOfPlayer.y, x_part, y_part);
						playersToDraw.push_back(playerPointPair);

						linesToDraw.push_back(PointPair(0, nearestPoints[0].p2.x, nearestPoints[0].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[1].p2.x, nearestPoints[1].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[2].p2.x, nearestPoints[2].p2.y, x_part, y_part));
					}

					classIds.push_back(classIdPoint.x);
					confidences.push_back((float)confidence);
					boxes.push_back(Rect(left, top, width, height));
				}
			}
		}
		ModelImageGenerator::createFieldModel(allPointPairs, linesToDraw, playersToDraw);

		std::vector<int> indices;
		NMSBoxes(boxes, confidences, confThreshold, 0.4, indices);
		for (size_t i = 0; i < indices.size(); ++i)
		{
			int idx = indices[i];
			Rect box = boxes[idx];
			drawPred(classIds[idx], confidences[idx], box.x, box.y,
				box.x + box.width, box.y + box.height, frame);
		}
	}
	else
		CV_Error(Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);
}

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
	rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

	std::string label = format("%.2f", conf);
	if (!classes.empty())
	{
		CV_Assert(classId < (int)classes.size());
		label = classes[classId] + ": " + label;
	}

	int baseLine;
	Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

	top = max(top, labelSize.height);
	rectangle(frame, Point(left, top - labelSize.height),
		Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
	putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
}

void callback(int pos, void*)
{
	confThreshold = pos * 0.01f;
}

std::vector<String> getOutputsNames(const Net& net)
{
	static std::vector<String> names;
	if (names.empty())
	{
		std::vector<int> outLayers = net.getUnconnectedOutLayers();
		std::vector<String> layersNames = net.getLayerNames();
		names.resize(outLayers.size());
		for (size_t i = 0; i < outLayers.size(); ++i)
			names[i] = layersNames[outLayers[i] - 1];
	}
	return names;
}

void initPointPairsMarcos() {

	// Oberste Linie
	//allPointPairs.push_back(PointPair(1, 646, 320, 0, 0)); // 1
	//allPointPairs.push_back(PointPair(2, 674, 320, 282, 0));
	allPointPairs.push_back(PointPair(3, 308, 715, 882, 0));
	allPointPairs.push_back(PointPair(4, 323, 665, 1182, 0));
	allPointPairs.push_back(PointPair(5, 336, 633, 1482, 0)); // 5
	allPointPairs.push_back(PointPair(6, 348, 598, 2082, 0));
	allPointPairs.push_back(PointPair(7, 355, 586, 2364, 0));
	// Zweite Linie
	//allPointPairs.push_back(PointPair(8, 615, 329, 0, 140));
	//allPointPairs.push_back(PointPair(9, 646, 334, 282, 140));
	allPointPairs.push_back(PointPair(10, 440, 712, 882, 140)); // 10
	allPointPairs.push_back(PointPair(11, 426, 664, 1182, 140));
	allPointPairs.push_back(PointPair(12, 415, 632, 1482, 140));
	allPointPairs.push_back(PointPair(13, 408, 598, 2082, 140));
	allPointPairs.push_back(PointPair(14, 407, 588, 2364, 140));
	// Dritte Linie
	allPointPairs.push_back(PointPair(15, 727, 809, 512, 284)); // 15
	allPointPairs.push_back(PointPair(16, 596, 702, 882, 284));
	allPointPairs.push_back(PointPair(17, 544, 658, 1182, 284));
	allPointPairs.push_back(PointPair(18, 514, 632, 1482, 284));
	allPointPairs.push_back(PointPair(19, 490, 607, 1852, 284));
	// Vierte Linie
	allPointPairs.push_back(PointPair(20, 1732, 904, 0, 590)); // 20
	allPointPairs.push_back(PointPair(21, 1267, 819, 282, 590));
	allPointPairs.push_back(PointPair(22, 601, 595, 2082, 590)); // A
	allPointPairs.push_back(PointPair(23, 582, 585, 2364, 590));
	// Fünfte Linie
	allPointPairs.push_back(PointPair(24, 1226, 709, 512, 896));
	allPointPairs.push_back(PointPair(25, 1019, 664, 882, 896)); // 25
	allPointPairs.push_back(PointPair(26, 912, 638, 1182, 896));
	allPointPairs.push_back(PointPair(27, 833, 618, 1482, 896));
	allPointPairs.push_back(PointPair(28, 764, 598, 1852, 896)); // B
	// Sechste Linie
	allPointPairs.push_back(PointPair(29, 1796, 777, 0, 1040));
	allPointPairs.push_back(PointPair(30, 1483, 726, 282, 1040)); // 30
	allPointPairs.push_back(PointPair(31, 1080, 656, 882, 1040));
	allPointPairs.push_back(PointPair(32, 967, 633, 1182, 1040));
	allPointPairs.push_back(PointPair(33, 890, 617, 1482, 1040));
	allPointPairs.push_back(PointPair(34, 776, 590, 2082, 1040)); // C
	// 35 sichtbar, aber bei hudritsch nicht drin
	// Siebte Linie
	allPointPairs.push_back(PointPair(36, 1804, 735, 0, 1180));
	allPointPairs.push_back(PointPair(37, 1522, 708, 282, 1180));
	allPointPairs.push_back(PointPair(38, 1127, 647, 882, 1180));
	allPointPairs.push_back(PointPair(39, 1016, 627, 1182, 1180));
	allPointPairs.push_back(PointPair(40, 938, 612, 1482, 1180)); // 40
	// Zweiter Teil vierte Linie
	// 41 + 42 sichtbar aber bei hudritsch nicht drin
	allPointPairs.push_back(PointPair(43, 1037, 749, 512, 590)); // 43
	allPointPairs.push_back(PointPair(44, 844, 681, 882, 590));
	allPointPairs.push_back(PointPair(45, 756, 647, 1182, 590));
	allPointPairs.push_back(PointPair(46, 690, 623, 1482, 590));
	allPointPairs.push_back(PointPair(47, 639, 605, 1852, 590));
}


void initPointPairsHudritsch() {
	// Oberste Linie
	allPointPairs.push_back(PointPair(1, 646, 320, 0, 0)); // 1
	allPointPairs.push_back(PointPair(2, 674, 320, 282, 0));
	allPointPairs.push_back(PointPair(3, 808, 329, 882, 0));
	allPointPairs.push_back(PointPair(4, 886, 335, 1182, 0));
	allPointPairs.push_back(PointPair(5, 983, 345, 1482, 0)); // 5
	allPointPairs.push_back(PointPair(6, 1281, 381, 2082, 0));
	allPointPairs.push_back(PointPair(7, 1486, 418, 2364, 0));
	// Zweite Linie
	allPointPairs.push_back(PointPair(8, 615, 329, 0, 140));
	allPointPairs.push_back(PointPair(9, 646, 334, 282, 140));
	allPointPairs.push_back(PointPair(10, 762, 344, 882, 140)); // 10
	allPointPairs.push_back(PointPair(11, 846, 351, 1182, 140));
	allPointPairs.push_back(PointPair(12, 949, 363, 1482, 140));
	allPointPairs.push_back(PointPair(13, 1257, 403, 2082, 140));
	allPointPairs.push_back(PointPair(14, 1485, 444, 2364, 140));
	// Dritte Linie
	allPointPairs.push_back(PointPair(15, 643, 350, 512, 284)); // 15
	allPointPairs.push_back(PointPair(16, 720, 359, 882, 284));
	allPointPairs.push_back(PointPair(17, 801, 367, 1182, 284));
	allPointPairs.push_back(PointPair(18, 905, 381, 1482, 284));
	allPointPairs.push_back(PointPair(19, 1078, 410, 1852, 284));
	// Vierte Linie
	allPointPairs.push_back(PointPair(20, 479, 370, 0, 590)); // 20
	allPointPairs.push_back(PointPair(21, 505, 375, 282, 590));
	allPointPairs.push_back(PointPair(22, 1128, 531, 2082, 590)); // A
	allPointPairs.push_back(PointPair(23, 1452, 610, 2364, 590));
	// Fünfte Linie
	allPointPairs.push_back(PointPair(24, 401, 426, 512, 896));
	allPointPairs.push_back(PointPair(25, 447, 450, 882, 896)); // 25
	allPointPairs.push_back(PointPair(26, 506, 479, 1182, 896));
	allPointPairs.push_back(PointPair(27, 593, 522, 1482, 896));
	allPointPairs.push_back(PointPair(28, 771, 614, 1852, 896)); // B
	// Sechste Linie
	allPointPairs.push_back(PointPair(29, 299, 422, 0, 1040));
	allPointPairs.push_back(PointPair(30, 313, 438, 282, 1040)); // 30
	allPointPairs.push_back(PointPair(31, 369, 482, 882, 1040));
	allPointPairs.push_back(PointPair(32, 410, 517, 1182, 1040));
	allPointPairs.push_back(PointPair(33, 481, 575, 1482, 1040));
	allPointPairs.push_back(PointPair(34, 824, 819, 2082, 1040)); // C
		 // 35 gibts nicht
	// Siebte Linie
	allPointPairs.push_back(PointPair(36, 236, 439, 0, 1180));
	allPointPairs.push_back(PointPair(37, 245, 461, 282, 1180));
	allPointPairs.push_back(PointPair(38, 276, 520, 882, 1180));
	allPointPairs.push_back(PointPair(39, 301, 566, 1182, 1180));
	allPointPairs.push_back(PointPair(40, 351, 637, 1482, 1180)); // 40
	// Zweiter Teil vierte Linie
	allPointPairs.push_back(PointPair(43, 531, 384, 512, 590)); // 43
	allPointPairs.push_back(PointPair(44, 598, 398, 882, 590));
	allPointPairs.push_back(PointPair(45, 673, 416, 1182, 590));
	allPointPairs.push_back(PointPair(46, 774, 439, 1482, 590));
	allPointPairs.push_back(PointPair(47, 960, 486, 1852, 590));
}


















