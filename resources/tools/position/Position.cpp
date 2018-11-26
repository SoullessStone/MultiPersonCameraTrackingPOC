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

std::vector<PointPair> allPointPairs;
int clickNumber = 1;

Mat frame;
Point pt(-1,-1);
bool newCoords = false;

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
    if (event == EVENT_LBUTTONDOWN)
    {
        // Store point coordinates
        pt.x = x;
        pt.y = y;
        newCoords = true;
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
		Mat frame = imread( argv[2]);
		cv::resize(frame,frame,Size((int)(((double)frame.cols / (double)2)),(int)(((double)frame.rows / (double)2))), 0, 0, cv::INTER_AREA);
		namedWindow("img", 1);
		setMouseCallback("img", mouse_callback);
		for (;;)
		{

			// Show last point clicked, if valid
			if (pt.x != -1 && pt.y != -1)
			{

			    if (newCoords)
			    {
				std::cout << clickNumber << " - Clicked coordinates: " << pt.x * 2 << ", " << pt.y * 2 << std::endl;
				newCoords = false;
				circle(frame, pt, 6, Scalar(0, 0, 255));
				putText(frame, std::to_string(clickNumber), cvPoint(pt.x+5,pt.y), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
				clickNumber++;
			    }
			}

			imshow("img", frame);

			// Exit if 'q' is pressed
			if ((waitKey(1) & 0xFF) == 'q') break;
		}
	} else if (arg == "Check")
	{
		std::vector<PointPair> testPointPairs;
		std::string arg2 = argv[2];
		if (arg2 == "Marcos")
		{
			// top line
			testPointPairs.push_back(PointPair(1, 1916, 956, 0, 0)); // 1
			testPointPairs.push_back(PointPair(2, 1500, 1068, 282, 0));
			testPointPairs.push_back(PointPair(3, 74, 1074, 882, 0));
			// 4 - 7 not visible
			// Second line
			testPointPairs.push_back(PointPair(8, 1806, 764, 0, 140));
			testPointPairs.push_back(PointPair(9, 1402, 834, 282, 140));
			testPointPairs.push_back(PointPair(10, 176, 828, 882, 140)); // 10
			// 11 - 14 not visible
			// Third line
			testPointPairs.push_back(PointPair(15, 952, 628, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 284, 616, 882, 284));
			// 17 - 19 not visible
			// Fourth line
			testPointPairs.push_back(PointPair(20, 1502, 364, 0, 590)); // 20
			testPointPairs.push_back(PointPair(21, 1190, 336, 282, 590));
			// 22 - 23 not visible
			// Fifth line
			testPointPairs.push_back(PointPair(24, 928, 182, 512, 896));
			testPointPairs.push_back(PointPair(25, 592, 176, 882, 896)); // 25
			testPointPairs.push_back(PointPair(26, 318, 186, 1182, 896));
			testPointPairs.push_back(PointPair(27, 78, 208, 1482, 896));
			// 28 not visible
			// Sixth line
			testPointPairs.push_back(PointPair(29, 1312, 180, 0, 1040));
			testPointPairs.push_back(PointPair(30, 1098, 156, 282, 1040)); // 30
			testPointPairs.push_back(PointPair(31, 628, 134, 882, 1040));
			testPointPairs.push_back(PointPair(32, 384, 148, 1182, 1040));
			testPointPairs.push_back(PointPair(33, 158, 168, 1482, 1040));
			// 34 & 35 not visible
			// Seventh line
			testPointPairs.push_back(PointPair(36, 1270, 144, 0, 1180));
			testPointPairs.push_back(PointPair(37, 1076, 118, 282, 1180));
			testPointPairs.push_back(PointPair(38, 666, 104, 882, 1180));
			testPointPairs.push_back(PointPair(39, 436, 112, 1182, 1180));
			testPointPairs.push_back(PointPair(40, 234, 128, 1482, 1180)); // 40
			testPointPairs.push_back(PointPair(41, 0, 152, 2082, 1180));
			// Second part Fourth line
			// 42 not visible
			testPointPairs.push_back(PointPair(43, 934, 320, 512, 590)); // 43
			testPointPairs.push_back(PointPair(44, 470, 312, 882, 590));
			testPointPairs.push_back(PointPair(45, 122, 326, 1182, 590));
			// 46 & 47 not visible
		} else if (arg2 == "Hudritsch")
		{
			// top line
			testPointPairs.push_back(PointPair(1, 240, 192, 0, 0)); // 1
			testPointPairs.push_back(PointPair(2, 386, 168, 282, 0));
			testPointPairs.push_back(PointPair(3, 754, 118, 882, 0));
			testPointPairs.push_back(PointPair(4, 950, 106, 1182, 0));
			testPointPairs.push_back(PointPair(5, 1146, 108, 1482, 0)); // 5
			testPointPairs.push_back(PointPair(6, 1554, 128, 2082, 0));
			testPointPairs.push_back(PointPair(7, 1714, 150, 2364, 0));
			// Second line
			testPointPairs.push_back(PointPair(8, 182, 220, 0, 140));
			testPointPairs.push_back(PointPair(9, 342, 200, 282, 140));
			testPointPairs.push_back(PointPair(10, 734, 148, 882, 140)); // 10
			testPointPairs.push_back(PointPair(11, 948, 140, 1182, 140));
			testPointPairs.push_back(PointPair(12, 1164, 136, 1482, 140));
			testPointPairs.push_back(PointPair(13, 1592, 162, 2082, 140));
			testPointPairs.push_back(PointPair(14, 1772, 182, 2364, 140));
			// Third line
			testPointPairs.push_back(PointPair(15, 416, 214, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 708, 188, 882, 284));
			testPointPairs.push_back(PointPair(17, 944, 174, 1182, 284));
			testPointPairs.push_back(PointPair(18, 1188, 170, 1482, 284));
			testPointPairs.push_back(PointPair(19, 1488, 188, 1852, 284));
			// Fourth line
			// 20 Not visible
			testPointPairs.push_back(PointPair(21, 116, 358, 282, 590));
			testPointPairs.push_back(PointPair(22, 1838, 322, 2082, 590)); // A
			// 23 not visible
			// Fifth line
			testPointPairs.push_back(PointPair(24, 88, 560, 512, 896));
			testPointPairs.push_back(PointPair(25, 494, 558, 882, 896)); // 25
			testPointPairs.push_back(PointPair(26, 922, 548, 1182, 896));
			testPointPairs.push_back(PointPair(27, 1372, 542, 1482, 896));
			testPointPairs.push_back(PointPair(28, 1856, 530, 1852, 896)); // B
			// Sixth line
			// 29&30 not visible
			testPointPairs.push_back(PointPair(31, 422, 732, 882, 1040));
			testPointPairs.push_back(PointPair(32, 910, 750, 1182, 1040));
			testPointPairs.push_back(PointPair(33, 1440, 732, 1482, 1040));
			// 34 - 37 not visible
			// Seventh line
			testPointPairs.push_back(PointPair(38, 324, 968, 882, 1180));
			testPointPairs.push_back(PointPair(39, 902, 1016, 1182, 1180));
			testPointPairs.push_back(PointPair(40, 1532, 990, 1482, 1180)); // 40
			// 41 not visible
			// Second part Fourth line
			// 42 not visible
			testPointPairs.push_back(PointPair(43, 288, 346, 512, 590)); // 43
			testPointPairs.push_back(PointPair(44, 628, 310, 882, 590));
			testPointPairs.push_back(PointPair(45, 930, 296, 1182, 590));
			testPointPairs.push_back(PointPair(46, 1258, 294, 1482, 590));
			testPointPairs.push_back(PointPair(47, 1636, 310, 1852, 590));
		}else if (arg2 == "Michel")
		{
			// top line
			testPointPairs.push_back(PointPair(1, 30, 94, 0, 0)); // 1
			testPointPairs.push_back(PointPair(2, 138, 86, 282, 0));
			testPointPairs.push_back(PointPair(3, 400, 74, 882, 0));
			testPointPairs.push_back(PointPair(4, 560, 72, 1182, 0));
			testPointPairs.push_back(PointPair(5, 742, 80, 1482, 0)); // 5
			testPointPairs.push_back(PointPair(6, 1132, 126, 2082, 0));
			testPointPairs.push_back(PointPair(7, 1338, 164, 2364, 0));
			// Second line
			// 8 not visible
			testPointPairs.push_back(PointPair(9, 64, 110, 282, 140));
			testPointPairs.push_back(PointPair(10, 348, 98, 882, 140)); // 10
			testPointPairs.push_back(PointPair(11, 516, 96, 1182, 140));
			testPointPairs.push_back(PointPair(12, 708, 106, 1482, 140));
			testPointPairs.push_back(PointPair(13, 1130, 162, 2082, 140));
			testPointPairs.push_back(PointPair(14, 1356, 204, 2364, 140));
			// Third line
			testPointPairs.push_back(PointPair(15, 80, 134, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 272, 130, 882, 284));
			testPointPairs.push_back(PointPair(17, 454, 130, 1182, 284));
			testPointPairs.push_back(PointPair(18, 666, 138, 1482, 284));
			testPointPairs.push_back(PointPair(19, 944, 170, 1852, 284));
			// Fourth line
			// 20 & 21 not visible
			testPointPairs.push_back(PointPair(22, 1126, 332, 2082, 590)); // A
			testPointPairs.push_back(PointPair(23, 1450, 396, 2364, 590));
			// Fifth line
			// 24 & 25 not visible
			testPointPairs.push_back(PointPair(26, 0, 388, 1182, 896));
			testPointPairs.push_back(PointPair(27, 290, 436, 1482, 896));
			testPointPairs.push_back(PointPair(28, 782, 526, 1852, 896)); // B
			// Sixth line
			// 29 - 32 not visible
			testPointPairs.push_back(PointPair(33, 132, 596, 1482, 1040));
			testPointPairs.push_back(PointPair(34, 1094, 790, 2082, 1040)); // C
			testPointPairs.push_back(PointPair(35, 1590, 846, 2364, 1040));
			// Seventh line
			// 36 - 39 not visible
			testPointPairs.push_back(PointPair(40, 0, 780, 1482, 1180)); // 40
			testPointPairs.push_back(PointPair(41, 1074, 1052, 2082, 1180));
			testPointPairs.push_back(PointPair(42, 1614, 1040, 2364, 1180));
			// Second part Fourth line
			// 43 not visible
			testPointPairs.push_back(PointPair(44, 64, 220, 882, 590));
			testPointPairs.push_back(PointPair(45, 266, 226, 1182, 590));
			testPointPairs.push_back(PointPair(46, 518, 240, 1482, 590));
			testPointPairs.push_back(PointPair(47, 890, 288, 1852, 590));
		} else {
			cout << "Bad params, check code for options" << endl;
		}


		Mat frame = imread( argv[3]);
		cv::resize(frame,frame,Size((int)(((double)frame.cols / (double)2)),(int)(((double)frame.rows / (double)2))), 0, 0, cv::INTER_AREA);
		for(PointPair& pp: testPointPairs) {
			circle(frame, pp.p1/2, 2, Scalar(0, 0, 255));		
			putText(frame, std::to_string(pp.id), cvPoint(pp.p1.x/2+5,pp.p1.y/2), FONT_HERSHEY_COMPLEX_SMALL, 0.4, cvScalar(200,200,250), 1, CV_AA);
		}
		imshow("img", frame);
		waitKey();
	} else {
	} 
}
	
