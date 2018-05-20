#include <fstream>
#include <sstream>
#include <iostream>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/xfeatures2d.hpp"

#include <stdio.h>
#include <stdlib.h>

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

using namespace cv;
using namespace dnn;
using namespace std;
using namespace cv::xfeatures2d;

void postprocess(Mat& frame, const std::vector<Mat>& out, Net& net);

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);

void callback(int pos, void* userdata);

std::vector<String> getOutputsNames(const Net& net);

int getPlayerColor(int, void*, Mat& playerImage);

void createNewField();

void showField();

void test(Mat& image);

void changeField(int x, int y, int newValue);

int countSiftMatches(Mat& player, Mat& number);
int getPossibilityForPlayerAndNumber(Mat& player, int number);

float confThreshold;
std::vector<std::string> classes;

const int width = 10;
const int height = 3;
std::array<std::array<int, width>, height> field;

int main(int argc, char** argv)
{
	CommandLineParser parser(argc, argv, keys);
	parser.about("Use this script to run object detection deep learning networks using OpenCV.");
	if (argc == 1 || parser.has("help"))
	{
		parser.printMessage();
		return 0;
	}

	// Init Gamefield
	createNewField();

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

	// Process frames.
	Mat frame, blob;
	while (waitKey(1) < 0)
	{
		cap >> frame;
		if (frame.empty())
		{
			waitKey();
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

		//imshow(kWinName, frame);
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
		cout << "Region" << endl;
		std::vector<int> classIds;
		std::vector<float> confidences;
		std::vector<Rect> boxes;
		for (size_t i = 0; i < outs.size(); ++i)
		{
			// Network produces output blob with a shape NxC where N is a number of
			// detected objects and C is a number of classes + 4 where the first 4
			// numbers are [center_x, center_y, width, height]
			float* data = (float*)outs[i].data;
			//static const std::string kWinName2 = "test";
			//namedWindow(kWinName2, WINDOW_NORMAL);
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

					// Handle Players
					if (classIdPoint.x == 0) {
						// Extract player from frame
						Mat player;
						player = frame(Rect(left, top, width, height));

						// Check if red or black player
						if (getPlayerColor(0, 0, player) == 1) {
							cout << "Red Player" << endl;

							// Find number
							cv::Mat greyPlayer;
							cv::cvtColor(player, greyPlayer, cv::COLOR_BGR2GRAY);
							std::array<int, 7> possibleNumbers = {1,3,4,5,6,8,9};
							for(int& i: possibleNumbers) { 
								cout << "------ Possibility for " << i << ": " << getPossibilityForPlayerAndNumber(greyPlayer, i) << endl;
								//waitKey();
							}
						}
						else {
							cout << "Black Player" << endl;
						}

						//imshow(kWinName2, player);
						waitKey(0);
					}

					classIds.push_back(classIdPoint.x);
					confidences.push_back((float)confidence);
					boxes.push_back(Rect(left, top, width, height));
				}
			}
		}
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

int getPossibilityForPlayerAndNumber(Mat& player, int number) {
	Mat n = imread( "numbers/"+std::to_string(number)+"_black.jpg", IMREAD_GRAYSCALE );

	int smallHeight = 40;
	int smallWidth = (int)((((double)n.cols / (double)n.rows))*(double)smallHeight);
	cv::resize(n,n,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);

	Mat nSmall;
	cv::resize(n,nSmall,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);
	int c1 = countSiftMatches(player, nSmall);

	smallHeight = 60;
	smallWidth = (int)((((double)n.cols / (double)n.rows))*(double)smallHeight);
	cv::resize(n,nSmall,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);
	int c2 = countSiftMatches(player, nSmall);

	smallHeight = 80;
	smallWidth = (int)((((double)n.cols / (double)n.rows))*(double)smallHeight);
	cv::resize(n,nSmall,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);
	int c3 = countSiftMatches(player, nSmall);

	smallHeight = 120;
	smallWidth = (int)((((double)n.cols / (double)n.rows))*(double)smallHeight);
	cv::resize(n,nSmall,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);
	int c4 = countSiftMatches(player, nSmall);

	smallHeight = 200;
	smallWidth = (int)((((double)n.cols / (double)n.rows))*(double)smallHeight);
	cv::resize(n,nSmall,Size(smallWidth,smallHeight), 0, 0, cv::INTER_AREA);
	int c5 = countSiftMatches(player, nSmall);

	// imwrite( "test.jpg", newPlayer );
	cout << "count 1: " << std::to_string(c1) << endl;
	cout << "count 2: " << std::to_string(c2) << endl;
	cout << "count 3: " << std::to_string(c3) << endl;
	cout << "count 4: " << std::to_string(c4) << endl;
	cout << "count 5: " << std::to_string(c5) << endl;
	return c1+c2+c3+c4+c5;
}

int countSiftMatches(Mat& player, Mat& number) {
	
	if( !player.data || !number.data )
	{
		std::cout<< " --(!) Error reading images " << std::endl;
		return -1; 
	}
	Mat resizedPlayer;
	
	int newHeight = 600;
	int newWidth = (int)((((double)player.cols / (double)player.rows))*(double)newHeight);
	cv::resize(player,resizedPlayer,Size(newWidth, newHeight), 0, 0, cv::INTER_AREA);

	//int left = resizedPlayer.cols / 4;
	//int top = resizedPlayer.rows / 6;
	//int width = resizedPlayer.cols / 2;
	//int height = resizedPlayer.rows / 2;
	//resizedPlayer = resizedPlayer(Rect(left, top, width, height));

	threshold( resizedPlayer, resizedPlayer, 120, 255,0 );

	imshow("before", resizedPlayer);

	cv::floodFill(resizedPlayer, cv::Point(0,0), 0, (cv::Rect*)0, cv::Scalar(), 200); 
	cv::floodFill(resizedPlayer, cv::Point(resizedPlayer.cols-1,resizedPlayer.rows-1), 0, (cv::Rect*)0, cv::Scalar(), 200); 
	cv::floodFill(resizedPlayer, cv::Point(resizedPlayer.cols-1,0), 0, (cv::Rect*)0, cv::Scalar(), 200); 
	cv::floodFill(resizedPlayer, cv::Point(0,resizedPlayer.rows-1), 0, (cv::Rect*)0, cv::Scalar(), 200); 

	int erosion_size = 1;
	Mat element = getStructuringElement( MORPH_RECT,
                                       Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                                       Point( erosion_size, erosion_size ) );
	erode( resizedPlayer, resizedPlayer, element );


	//-- Step 1: Detect the keypoints using SIFT Detector, compute the descriptors
	Ptr<SIFT> detector = SIFT::create();
	std::vector<KeyPoint> keypoints_1, keypoints_2;
	Mat descriptors_1, descriptors_2;
	detector->detectAndCompute( resizedPlayer, Mat(), keypoints_1, descriptors_1 );
	// TODO only calculate Keypoints&Descriptors once
	detector->detectAndCompute( number, Mat(), keypoints_2, descriptors_2 );
	//-- Step 2: Matching descriptor vectors using FLANN matcher
	BFMatcher matcher;
	std::vector< DMatch > matches;
	matcher.match( descriptors_1, descriptors_2, matches );
	double max_dist = 0; double min_dist = 130;
	//-- Quick calculation of max and min distances between keypoints
	for( int i = 0; i < descriptors_1.rows; i++ )
	{ double dist = matches[i].distance;
	if( dist < min_dist ) min_dist = dist;
	if( dist > max_dist ) max_dist = dist;
	}
	//printf("-- Max dist : %f \n", max_dist );
	//printf("-- Min dist : %f \n", min_dist );
	//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
	//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
	//-- small)
	//-- PS.- radiusMatch can also be used here.
	std::vector< DMatch > good_matches;
	for( int i = 0; i < descriptors_1.rows; i++ )
	{ 
		//printf("%f, min dist*2 = %f \n", matches[i].distance,2*min_dist);
		if( matches[i].distance <= max(2*min_dist, 0.02) )
		{ 
			good_matches.push_back( matches[i]); 
		}
	}
	//-- Draw only "good" matches
	Mat img_matches;
	drawMatches( resizedPlayer, keypoints_1, number, keypoints_2,
		 good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
		 vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
	//-- Show detected matches
	imshow( "Good Matches", img_matches );
	waitKey();
	return (int)good_matches.size();
	//for( int i = 0; i < (int)good_matches.size(); i++ )
	//{ 
		//printf( "-- Good Match [%d] Keypoint 1: %d-- Keypoint 2: %d\n", i, good_matches[i].queryIdx, good_matches[i].trainIdx ); 
	//}
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

// TODO: Eleganter lösen (frei wählbare Farben für beide Teams, count und vergleichen)
int getPlayerColor(int, void*, Mat& playerImage)
{
	Mat dst;//dst image
	resize(playerImage, dst, Size(100, 100));//resize image
	int count_red = 0;
	for (int y = 0; y < dst.rows; y++) {
		for (int x = 0; x < dst.cols; x++) {
			//if (dst.at<uchar>(y, x) != 0) {
				if (dst.at<cv::Vec3b>(y, x)[2] > 150) {
					count_red++;
				}
			//}
		}
	}
	float percent = (float)((float)count_red / ((float)dst.rows*(float)dst.cols));
	//cout << count_red << "/" << (dst.rows*dst.cols) << " = " << percent << endl;
	if (percent > 0.027) {
		return 1;
	}
	return 0;
}

void createNewField() {
	std::array<std::array<int, width>, height> arr;
	
	for ( int i = 0; i < height; i++ ) {
		std::array<int, width> row = arr.at(i);
		for ( int j = 0; j < width; j++ ) {
			row.at(j) = 0;
			//cout << "i, j = " << i << ", " << j << endl;
			//cout << row.at(j) << endl;
		}
		arr.at(i) = row;
	}
	field = arr;
	changeField(5,1,1);
	showField();
}

void showField() {
	
	Mat image = Mat(height*5, width*5, CV_8UC3, Scalar(0,0,255));
	for ( int i = 0; i < height; i++ ) {
		std::array<int, width> row = field.at(i);
		for ( int j = 0; j < width; j++ ) {
			// Draw each part of the field a little bigger
			for (int ii = i*5; ii < i*5+5; ii++) {
				for (int jj = j*5; jj < j*5+5; jj++) {
					// Color Player
					if (row.at(j) == 1) {
						image.at<cv::Vec3b>(ii,jj)[2] = 100;
					}
				}
			}
			cout << row[j] << " ";
			
		}
		cout << endl;
	}
	static const std::string kWinName3 = "Field";
	//namedWindow(kWinName3, WINDOW_NORMAL);
	//imshow(kWinName3, image);
}

void changeField(int x, int y, int newValue) {
	field.at(y).at(x) = newValue;
}





















