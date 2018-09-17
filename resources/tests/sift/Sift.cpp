/*
 * @file SURF_FlannMatcher
 * @brief SURF detector + descriptor + FLANN Matcher
 * @author A. Huaman
 */
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
using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

void readme();
int countSiftMatches(Mat& player, Mat& number);
int getPossibilityForPlayerAndNumber(Mat& player, int number);

/*
 * @function main
 * @brief Main function
 */
int main( int argc, char** argv )
{
	if( argc != 1 )
	{ readme(); return -1; }

	std::array<std::string, 22> testImages = {
		"testimages/1_1.PNG",
		"testimages/1_2.PNG",
		"testimages/1_3.PNG",
		"testimages/1_4.PNG",
		"testimages/4_1.PNG",
		"testimages/4_2.PNG",
		"testimages/4_3.PNG",
		"testimages/6_1.PNG",
		"testimages/6_2.PNG",
		"testimages/6_3.PNG",
		"testimages/6_4.PNG",
		"testimages/6_5.PNG",
		"testimages/8_1.PNG",
		"testimages/8_2.PNG",
		"testimages/8_3.PNG",
		"testimages/8_4.PNG",
		"testimages/9_1.PNG",
		"testimages/9_2.PNG",
		"testimages/9_3.PNG",
		"testimages/9_4.PNG",
		"testimages/9_5.PNG",
		"testimages/9_6.PNG"
	};
	// Nur Spieler gethresholded: 0.136364
	// Spieler und Nummern gethressholded: 0.227273
	/*std::array<std::string, 22> testImages = {
		"testimages/1_1_t.PNG",
		"testimages/1_2_t.PNG",
		"testimages/1_3_t.PNG",
		"testimages/1_4_t.PNG",
		"testimages/4_1_t.PNG",
		"testimages/4_2_t.PNG",
		"testimages/4_3_t.PNG",
		"testimages/6_1_t.PNG",
		"testimages/6_2_t.PNG",
		"testimages/6_3_t.PNG",
		"testimages/6_4_t.PNG",
		"testimages/6_5_t.PNG",
		"testimages/8_1_t.PNG",
		"testimages/8_2_t.PNG",
		"testimages/8_3_t.PNG",
		"testimages/8_4_t.PNG",
		"testimages/9_1_t.PNG",
		"testimages/9_2_t.PNG",
		"testimages/9_3_t.PNG",
		"testimages/9_4_t.PNG",
		"testimages/9_5_t.PNG",
		"testimages/9_6_t.PNG"
	};*/

	int correct = 0;
	int wrong = 0;
	for(std::string& s: testImages) {
		cout << "--- " << s << endl;
		Mat player = imread( s, IMREAD_GRAYSCALE );
		// resize player: 0.227273
		//cv::resize(player,player,Size(player.cols*4,player.rows*4), 0, 0, cv::INTER_AREA);
	
		std::array<int, 7> possibleNumbers = {1,3,4,5,6,8,9};
		int maxNumber = 0;
		int maxCount = 0;
		for(int& i: possibleNumbers) { 
			int temp = getPossibilityForPlayerAndNumber(player, i);
			cout << "------ Possibility for " << i << ": " << temp << endl;
			if (temp > maxNumber) {
				maxNumber = i;
				maxCount = temp;
			}
		}
		cout << "This is number " << maxNumber << " (" << maxCount << ")" << endl;
		cout << "Should be number " << s.substr (11,1) << endl;
		if (maxCount >= 10) {
			if (std::stoi( s.substr (11,1) ) == maxNumber) {
				correct++;
			}else {
				wrong++;
			}
		} else {
			cout << "Skipped, not sure enough" << endl;
		}		
		//imshow("Player", player);
		//waitKey();
	}

	cout << " ===================================================== " << endl;
	cout << correct << "/" << correct+wrong << " = " << (float)((float)correct / ((float)wrong + (float)correct)) << endl;
	// Normale Konfiguration: 0.318182

return 0;
}

int getPossibilityForPlayerAndNumber(Mat& player, int number) {
	cout << player.cols << "x" << player.rows << endl;
	// Nur Nummern gethresholded: 0.272727
	// Mat n = imread( "numbers/"+std::to_string(number)+"_t.jpg", IMREAD_GRAYSCALE );
	Mat n = imread( "numbers/"+std::to_string(number)+".jpg", IMREAD_GRAYSCALE );
	Mat nSmall;
	cv::resize(n,nSmall,Size(45,60), 0, 0, cv::INTER_AREA);
	int c1 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(60,80), 0, 0, cv::INTER_AREA);
	int c2 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(90,120), 0, 0, cv::INTER_AREA);
	int c3 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(112,150), 0, 0, cv::INTER_AREA);
	int c4 = countSiftMatches(player, nSmall);
	//cout << "count autoscaled: " << std::to_string(c) << endl;
	cout << number << ": possibilities -> " << c1 << ", "<< c2 << ", "<< c3 << ", "<< c4 << endl;
	return c1+c2+c3+c4;
}

int countSiftMatches(Mat& player, Mat& number) {
	
	if( !player.data || !number.data )
	{
		std::cout<< " --(!) Error reading images " << std::endl;
		return -1; 
	}
	//-- Step 1: Detect the keypoints using SIFT Detector, compute the descriptors
	Ptr<SIFT> detector = SIFT::create();
	std::vector<KeyPoint> keypoints_1, keypoints_2;
	Mat descriptors_1, descriptors_2;
	detector->detectAndCompute( player, Mat(), keypoints_1, descriptors_1 );
	// TODO only calculate Keypoints&Descriptors once
	detector->detectAndCompute( number, Mat(), keypoints_2, descriptors_2 );
	//-- Step 2: Matching descriptor vectors using FLANN matcher
	BFMatcher matcher;
	std::vector< DMatch > matches;
	matcher.match( descriptors_1, descriptors_2, matches );
	double max_dist = 0; double min_dist = 100;
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
	drawMatches( player, keypoints_1, number, keypoints_2,
		 good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
		 vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
	//-- Show detected matches
	//imshow( "Good Matches", img_matches );
	return (int)good_matches.size();
	//for( int i = 0; i < (int)good_matches.size(); i++ )
	//{ 
		//printf( "-- Good Match [%d] Keypoint 1: %d-- Keypoint 2: %d\n", i, good_matches[i].queryIdx, good_matches[i].trainIdx ); 
	//}
}

/*
 * @function readme
 */
void readme()
{ std::cout << " Usage: ./SURF_FlannMatcher <img1> <img2>" << std::endl; }
