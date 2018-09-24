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
			// PointPairs to Check
			// 1 nicht sichtbar
			testPointPairs.push_back(PointPair(2, 214, 1066, 282, 0));
			testPointPairs.push_back(PointPair(3, 308, 715, 882, 0));
			testPointPairs.push_back(PointPair(4, 323, 665, 1182, 0));
			testPointPairs.push_back(PointPair(5, 336, 633, 1482, 0)); // 5
			testPointPairs.push_back(PointPair(6, 348, 598, 2082, 0));
			testPointPairs.push_back(PointPair(7, 355, 586, 2364, 0));
			// Zweite Linie
			// 8 nicht sichtbar
			testPointPairs.push_back(PointPair(9, 584, 1026, 282, 140));
			testPointPairs.push_back(PointPair(10, 440, 712, 882, 140)); // 10
			testPointPairs.push_back(PointPair(11, 426, 664, 1182, 140));
			testPointPairs.push_back(PointPair(12, 415, 632, 1482, 140));
			testPointPairs.push_back(PointPair(13, 408, 598, 2082, 140));
			testPointPairs.push_back(PointPair(14, 407, 588, 2364, 140));
			// Dritte Linie
			testPointPairs.push_back(PointPair(15, 727, 809, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 596, 702, 882, 284));
			testPointPairs.push_back(PointPair(17, 544, 658, 1182, 284));
			testPointPairs.push_back(PointPair(18, 514, 632, 1482, 284));
			testPointPairs.push_back(PointPair(19, 490, 607, 1852, 284));
			// Vierte Linie
			testPointPairs.push_back(PointPair(20, 1732, 904, 0, 590)); // 20
			testPointPairs.push_back(PointPair(21, 1267, 819, 282, 590));
			testPointPairs.push_back(PointPair(22, 601, 595, 2082, 590)); // A
			testPointPairs.push_back(PointPair(23, 582, 585, 2364, 590));
			// Fünfte Linie
			testPointPairs.push_back(PointPair(24, 1226, 709, 512, 896));
			testPointPairs.push_back(PointPair(25, 1019, 664, 882, 896)); // 25
			testPointPairs.push_back(PointPair(26, 912, 638, 1182, 896));
			testPointPairs.push_back(PointPair(27, 833, 618, 1482, 896));
			testPointPairs.push_back(PointPair(28, 764, 598, 1852, 896)); // B
			// Sechste Linie
			testPointPairs.push_back(PointPair(29, 1796, 777, 0, 1040));
			testPointPairs.push_back(PointPair(30, 1483, 726, 282, 1040)); // 30
			testPointPairs.push_back(PointPair(31, 1080, 656, 882, 1040));
			testPointPairs.push_back(PointPair(32, 967, 633, 1182, 1040));
			testPointPairs.push_back(PointPair(33, 890, 617, 1482, 1040));
			testPointPairs.push_back(PointPair(34, 776, 590, 2082, 1040)); // C
			testPointPairs.push_back(PointPair(35, 748, 580, 2364, 1040));
			// Siebte Linie
			testPointPairs.push_back(PointPair(36, 1804, 735, 0, 1180));
			testPointPairs.push_back(PointPair(37, 1522, 708, 282, 1180));
			testPointPairs.push_back(PointPair(38, 1127, 647, 882, 1180));
			testPointPairs.push_back(PointPair(39, 1016, 627, 1182, 1180));
			testPointPairs.push_back(PointPair(40, 938, 612, 1482, 1180)); // 40
			// Zweiter Teil vierte Linie
			testPointPairs.push_back(PointPair(41, 814, 586, 2082, 1180));
			testPointPairs.push_back(PointPair(42, 776, 576, 2364, 1180));
			testPointPairs.push_back(PointPair(43, 1037, 749, 512, 590)); // 43
			testPointPairs.push_back(PointPair(44, 844, 681, 882, 590));
			testPointPairs.push_back(PointPair(45, 756, 647, 1182, 590));
			testPointPairs.push_back(PointPair(46, 690, 623, 1482, 590));
			testPointPairs.push_back(PointPair(47, 639, 605, 1852, 590));
		} else if (arg2 == "Hudritsch")
		{
			// Oberste Linie
			testPointPairs.push_back(PointPair(1, 646, 320, 0, 0)); // 1
			testPointPairs.push_back(PointPair(2, 674, 320, 282, 0));
			testPointPairs.push_back(PointPair(3, 808, 329, 882, 0));
			testPointPairs.push_back(PointPair(4, 886, 335, 1182, 0));
			testPointPairs.push_back(PointPair(5, 983, 345, 1482, 0)); // 5
			testPointPairs.push_back(PointPair(6, 1281, 381, 2082, 0));
			testPointPairs.push_back(PointPair(7, 1486, 418, 2364, 0));
			// Zweite Linie
			testPointPairs.push_back(PointPair(8, 615, 329, 0, 140));
			testPointPairs.push_back(PointPair(9, 646, 334, 282, 140));
			testPointPairs.push_back(PointPair(10, 762, 344, 882, 140)); // 10
			testPointPairs.push_back(PointPair(11, 846, 351, 1182, 140));
			testPointPairs.push_back(PointPair(12, 949, 363, 1482, 140));
			testPointPairs.push_back(PointPair(13, 1257, 403, 2082, 140));
			testPointPairs.push_back(PointPair(14, 1485, 444, 2364, 140));
			// Dritte Linie
			testPointPairs.push_back(PointPair(15, 643, 350, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 720, 359, 882, 284));
			testPointPairs.push_back(PointPair(17, 801, 367, 1182, 284));
			testPointPairs.push_back(PointPair(18, 905, 381, 1482, 284));
			testPointPairs.push_back(PointPair(19, 1078, 410, 1852, 284));
			// Vierte Linie
			testPointPairs.push_back(PointPair(20, 479, 370, 0, 590)); // 20
			testPointPairs.push_back(PointPair(21, 505, 375, 282, 590));
			testPointPairs.push_back(PointPair(22, 1128, 531, 2082, 590)); // A
			testPointPairs.push_back(PointPair(23, 1452, 610, 2364, 590));
			// Fünfte Linie
			testPointPairs.push_back(PointPair(24, 401, 426, 512, 896));
			testPointPairs.push_back(PointPair(25, 447, 450, 882, 896)); // 25
			testPointPairs.push_back(PointPair(26, 506, 479, 1182, 896));
			testPointPairs.push_back(PointPair(27, 593, 522, 1482, 896));
			testPointPairs.push_back(PointPair(28, 771, 614, 1852, 896)); // B
			// Sechste Linie
			testPointPairs.push_back(PointPair(29, 299, 422, 0, 1040));
			testPointPairs.push_back(PointPair(30, 313, 438, 282, 1040)); // 30
			testPointPairs.push_back(PointPair(31, 369, 482, 882, 1040));
			testPointPairs.push_back(PointPair(32, 410, 517, 1182, 1040));
			testPointPairs.push_back(PointPair(33, 481, 575, 1482, 1040));
			testPointPairs.push_back(PointPair(34, 824, 819, 2082, 1040)); // C
			testPointPairs.push_back(PointPair(35, 1276, 1040, 2364, 1040));
			// Siebte Linie
			testPointPairs.push_back(PointPair(36, 236, 439, 0, 1180));
			testPointPairs.push_back(PointPair(37, 245, 461, 282, 1180));
			testPointPairs.push_back(PointPair(38, 276, 520, 882, 1180));
			testPointPairs.push_back(PointPair(39, 301, 566, 1182, 1180));
			testPointPairs.push_back(PointPair(40, 351, 637, 1482, 1180)); // 40
			testPointPairs.push_back(PointPair(41, 652, 970, 1482, 1180));
			// Zweiter Teil vierte Linie
			// 42 nicht sichtbar
			testPointPairs.push_back(PointPair(43, 531, 384, 512, 590)); // 43
			testPointPairs.push_back(PointPair(44, 598, 398, 882, 590));
			testPointPairs.push_back(PointPair(45, 673, 416, 1182, 590));
			testPointPairs.push_back(PointPair(46, 774, 439, 1482, 590));
			testPointPairs.push_back(PointPair(47, 960, 486, 2082, 590));
		}else if (arg2 == "Michel")
		{
			testPointPairs.push_back(PointPair(1, 292, 361, 0, 0)); // 1
			testPointPairs.push_back(PointPair(2, 457, 342, 282, 0));
			testPointPairs.push_back(PointPair(3, 884, 303, 882, 0));
			testPointPairs.push_back(PointPair(4, 1063, 295, 1182, 0));
			testPointPairs.push_back(PointPair(5, 1229, 293, 1482, 0)); // 5
			testPointPairs.push_back(PointPair(6, 1527, 296, 2082, 0));
			testPointPairs.push_back(PointPair(7, 1618, 297, 2364, 0));
			// Zweite Linie
			testPointPairs.push_back(PointPair(8, 263, 380, 0, 140));
			testPointPairs.push_back(PointPair(9, 447, 367, 282, 140));
			testPointPairs.push_back(PointPair(10, 892, 329, 882, 140)); // 10
			testPointPairs.push_back(PointPair(11, 1087, 318, 1182, 140));
			testPointPairs.push_back(PointPair(12, 1266, 312, 1482, 140));
			testPointPairs.push_back(PointPair(13, 1559, 315, 2082, 140));
			testPointPairs.push_back(PointPair(14, 1662, 315, 2364, 140));
			// Dritte Linie
			testPointPairs.push_back(PointPair(15, 601, 390, 512, 284)); // 15
			testPointPairs.push_back(PointPair(16, 904, 363, 882, 284));
			testPointPairs.push_back(PointPair(17, 1121, 352, 1182, 284));
			testPointPairs.push_back(PointPair(18, 1316, 346, 1482, 284));
			testPointPairs.push_back(PointPair(19, 1520, 340, 1852, 284));
			// Vierte Linie
			testPointPairs.push_back(PointPair(20, 5, 580, 0, 590)); // 20
			testPointPairs.push_back(PointPair(21, 279, 555, 282, 590));
			testPointPairs.push_back(PointPair(22, 1811, 424, 2082, 590)); // A
			testPointPairs.push_back(PointPair(23, 1898, 419, 2364, 590));
			// Fünfte Linie
			testPointPairs.push_back(PointPair(24, 436, 794, 512, 896));
			testPointPairs.push_back(PointPair(25, 1028, 728, 882, 896)); // 25
			testPointPairs.push_back(PointPair(26, 1424, 664, 1182, 896));
			testPointPairs.push_back(PointPair(27, 1705, 611, 1482, 896));
			testPointPairs.push_back(PointPair(28, 1919, 561, 1852, 896)); // B
			// Sechste Linie
			// 29 nicht sichtbar
			testPointPairs.push_back(PointPair(30, 2, 962, 282, 1040)); // 30
			testPointPairs.push_back(PointPair(31, 1094, 914, 882, 1040));
			testPointPairs.push_back(PointPair(32, 1558, 808, 1182, 1040));
			testPointPairs.push_back(PointPair(33, 1860, 716, 1482, 1040));
			// 34 nicht sichtbar
			// 35 nicht sichtbar
			// Siebte Linie
			// 36 nicht sichtbar
			// 37 nicht sichtbar
			testPointPairs.push_back(PointPair(37, 1622, 2, 282, 1180));
			testPointPairs.push_back(PointPair(38, 1166, 1078, 882, 1180));
			testPointPairs.push_back(PointPair(39, 1684, 930, 1182, 1180));
			// 40 nicht sichtbar
			// Zweiter Teil vierte Linie
			// 41 nicht sichtbar
			// 42 nicht sichtbar
			testPointPairs.push_back(PointPair(43, 536, 528, 512, 590)); // 43
			testPointPairs.push_back(PointPair(44, 948, 490, 882, 590));
			testPointPairs.push_back(PointPair(45, 1232, 460, 1182, 590));
			testPointPairs.push_back(PointPair(46, 1472, 446, 1482, 590));
			testPointPairs.push_back(PointPair(47, 1700, 428, 1852, 590));
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
	
