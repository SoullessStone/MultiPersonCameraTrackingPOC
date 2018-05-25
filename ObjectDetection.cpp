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

// TODO Move to .h file

void postprocess(Mat& frame, const std::vector<Mat>& out, Net& net);

void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);

void callback(int pos, void* userdata);

std::vector<String> getOutputsNames(const Net& net);

int getPlayerColor(int, void*, Mat& playerImage);

void test(Mat& image);

int countSiftMatches(Mat& player, Mat& number);

int getPossibilityForPlayerAndNumber(Mat& player, int number);

void initPointPairs();

void barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w);

void createFieldModel(std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue);

std::array<PointPair, 3> findNearestThreePoints(Point p);

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
		waitKey(0);
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
		int counter = 0;
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
					int bottom = centerY + height / 2;

					// Handle Players
					if (classIdPoint.x == 0) {
						cout << "-------------------------------- Player " << counter << " --------------------------------" << endl;
						counter += 1;
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

						Point bottomOfPlayer(centerX, bottom);
						initPointPairs();
						std::array<PointPair, 3> nearestPoints = findNearestThreePoints(bottomOfPlayer);

						cout << "bottomOfPlayer: " << bottomOfPlayer << endl;
						float alpha = 0.0;
						float beta = 0.0;
						float gamma = 0.0;
						barycentric(bottomOfPlayer, nearestPoints[0].p1, nearestPoints[1].p1, nearestPoints[2].p1, alpha, beta, gamma);
						float x_part = alpha*(float)nearestPoints[0].p2.x + beta*(float)nearestPoints[1].p2.x + gamma*(float)nearestPoints[2].p2.x;
						float y_part = alpha*(float)nearestPoints[0].p2.y + beta*(float)nearestPoints[1].p2.y + gamma*(float)nearestPoints[2].p2.y;
						cout << "x: " << x_part << endl;
						cout << "y: " << y_part << endl;

						PointPair clickedPointPair(1000+j, bottomOfPlayer.x, bottomOfPlayer.y, x_part, y_part);

						std::vector<PointPair> input2;
						std::vector<PointPair> input3;
						input2.push_back(nearestPoints[0]);
						input2.push_back(nearestPoints[1]);
						input2.push_back(nearestPoints[2]);
						input3.push_back(clickedPointPair);
						createFieldModel(allPointPairs, input2, input3);


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
	/*cout << "count 1: " << std::to_string(c1) << endl;
	cout << "count 2: " << std::to_string(c2) << endl;
	cout << "count 3: " << std::to_string(c3) << endl;
	cout << "count 4: " << std::to_string(c4) << endl;
	cout << "count 5: " << std::to_string(c5) << endl;*/
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

	//imshow("before", resizedPlayer);

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
	//imshow( "Good Matches", img_matches );
	//waitKey();
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

// TODO: Eleganter lösen (frei wählbare Farben für beide Teams, count und vergleichen, evtl hsv-distance: http://answers.opencv.org/question/127885/how-can-i-best-compare-two-bgr-colors-to-determine-how-similardifferent-they-are/)
int getPlayerColor(int, void*, Mat& playerImage)
{
	Mat dst;//dst image
	resize(playerImage, dst, Size(100, 100));//resize image
	int count_red = 0;
	for (int y = 0; y < dst.rows; y++) {
		for (int x = 0; x < dst.cols; x++) {
			int r = dst.at<cv::Vec3b>(y, x)[2];
			int g = dst.at<cv::Vec3b>(y, x)[1];
			int b = dst.at<cv::Vec3b>(y, x)[0];
			bool verify = r + 50 > g+b;
			if (r > 150 && verify) {
				count_red++;
				//cout << dst.at<cv::Vec3b>(y, x) << endl;
				//waitKey();
			}
		}
	}
	float percent = (float)((float)count_red / ((float)dst.rows*(float)dst.cols));
	imshow("player", playerImage);
	cout << count_red << "/" << (dst.rows*dst.cols) << " = " << percent << endl;
	//waitKey();
	if (percent > 0.0) {
		return 1;
	}
	return 0;
}

void initPointPairs() {
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

void barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w)
{
  	int v0[] = { b.x-a.x, b.y-a.y };
  	int v1[] = { c.x-a.x, c.y-a.y };
  	int v2[] = { p.x-a.x, p.y-a.y };
	int array_size = 2;
	float d00 = inner_product(v0, v0 + array_size, v0, 0);
	float d01 = inner_product(v0, v0 + array_size, v1, 0);
	float d11 = inner_product(v1, v1 + array_size, v1, 0);
	float d20 = inner_product(v2, v2 + array_size, v0, 0);
	float d21 = inner_product(v2, v2 + array_size, v1, 0);
	float denom = d00 * d11 - d01 * d01;
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
}

void createFieldModel(std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue) {
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
	line(field, yellow_topleft, yellow_bottomleft, yellow_color, 2);
	line(field, yellow_topleft, yellow_topright, yellow_color, 2);
	line(field, yellow_topright, yellow_bottomright, yellow_color, 2);
	line(field, yellow_bottomleft, yellow_bottomright, yellow_color, 2);

	for(PointPair& pp: additionalPointsRed) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 8, Scalar(0, 0, 255));
		putText(field, std::to_string(pp.id), cvPoint(pp.p2.x / 2+15,pp.p2.y / 2+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
	}

	for(PointPair& pp: additionalPointsGreen) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 8, Scalar(0, 255, 0));
		putText(field, std::to_string(pp.id), cvPoint(pp.p2.x / 2+15,pp.p2.y / 2+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(100,200,250), 1, CV_AA);
	}

	for(PointPair& pp: additionalPointsBlue) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 8, Scalar(255, 0, 0));
		putText(field, std::to_string(pp.id), cvPoint(pp.p2.x / 2+15,pp.p2.y / 2+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,200,250), 1, CV_AA);
	}	

	imshow("Field-Model", field);
	imwrite( "test.jpg", field );
}

std::array<PointPair, 3> findNearestThreePoints(Point p) {
	//cout << "Input p: " << p << endl;

	PointPair nearestPP1(9999,9999,9999,9999,9999);
	double pp1Distance = 9999.0;
	PointPair nearestPP2(9999,9999,9999,9999,9999);
	double pp2Distance = 9999.0;
	PointPair nearestPP3(99999,999,9999,9999,9999);
	double pp3Distance = 9999.0;

	double maxDouble = std::numeric_limits<double>::max();
	int x, y;
	double sum1 = maxDouble, sum2 = maxDouble, sum3 = maxDouble;
	for(PointPair& curPP: allPointPairs) {
		x = p.x - curPP.p1.x;
		y = p.y - curPP.p1.y;
		double curDistance = sqrt(pow((double)x, 2.0) + pow((double)y, 2.0));
		//cout << "curPP: #" << curPP.id << " " << curPP.p1 << ", distance: " << curDistance << endl;

		if (pp1Distance == 9999.0) {
			nearestPP1 = curPP;
			pp1Distance = curDistance;
			continue;
		}
		if (pp2Distance == 9999.0) {
			nearestPP2 = curPP;
			pp2Distance = curDistance;
			continue;
		}
		if (pp3Distance == 9999.0) {
			nearestPP3 = curPP;
			pp3Distance = curDistance;
			continue;
		}
		bool pp1DistanceIsTheBiggest = pp1Distance >= pp2Distance && pp1Distance >= pp3Distance;
		if (pp1DistanceIsTheBiggest && curDistance < pp1Distance) {
			nearestPP1 = curPP;
			pp1Distance = curDistance;
			continue;
		}
		bool pp2DistanceIsTheBiggest = pp2Distance >= pp1Distance && pp2Distance >= pp3Distance;
		if (pp2DistanceIsTheBiggest && curDistance < pp2Distance) {
			nearestPP2 = curPP;
			pp2Distance = curDistance;
			continue;
		}
		bool pp3DistanceIsTheBiggest = pp3Distance >= pp2Distance && pp3Distance >= pp1Distance;
		if (pp3DistanceIsTheBiggest && curDistance < pp3Distance) {
			nearestPP3 = curPP;
			pp3Distance = curDistance;
			continue;
		}
		if (!(pp1DistanceIsTheBiggest||pp2DistanceIsTheBiggest||pp3DistanceIsTheBiggest))
		{
			cout << "Komischer State, mal reinschauen..." << endl;
		}
	}

	std::array<PointPair, 3> result = {
		nearestPP1, nearestPP2, nearestPP3
	};
	return result;
}





















