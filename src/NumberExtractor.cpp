#include <NumberExtractor.h>

int NumberExtractor::getNumberForPlayer(Mat& player)
{
	std::array<int, 7> possibleNumbers = {1,3,4,5,6,8,9};

	int maxNumber = 0;
	int maxCount = 0;
	for(int& i: possibleNumbers) { 
		int temp = NumberExtractor::getPossibilityForPlayerAndNumber(player, i);
		//cout << "------ Possibility for " << i << ": " << temp << endl;
		if (temp > maxCount) {
			maxNumber = i;
			maxCount = temp;
		}
	}
	if (maxCount >= 10) {
		return maxNumber;
		//cout << "This is number " << maxNumber << " (" << maxCount << ")" << endl;
	} else {
		//cout << "Skipped, not sure enough" << endl;
		return -1;
	}
}

int NumberExtractor::getPossibilityForPlayerAndNumber(Mat& player, int number)
{
	Mat n = imread( "../resources/numbers/"+std::to_string(number)+"_black.jpg", IMREAD_GRAYSCALE );
	Mat nSmall;
	cv::resize(n,nSmall,Size(45,60), 0, 0, cv::INTER_AREA);
	int c1 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(60,80), 0, 0, cv::INTER_AREA);
	int c2 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(90,120), 0, 0, cv::INTER_AREA);
	int c3 = countSiftMatches(player, nSmall);
	cv::resize(n,nSmall,Size(112,150), 0, 0, cv::INTER_AREA);
	int c4 = countSiftMatches(player, nSmall);

	//cout << number << ": possibilities -> " << c1 << ", "<< c2 << ", "<< c3 << ", "<< c4 << endl;
	return c1+c2+c3+c4;
}

int NumberExtractor::countSiftMatches(Mat& player, Mat& number)
{
	/*
	if( !player.data || !number.data )
	{
		Logger::log("Could not read images", 1);
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
	{
		double dist = matches[i].distance;
		if( dist < min_dist ) min_dist = dist;
		if( dist > max_dist ) max_dist = dist;
	}
	std::vector< DMatch > good_matches;
	for( int i = 0; i < descriptors_1.rows; i++ )
	{ 
		//printf("%f, min dist*2 = %f \n", matches[i].distance,2*min_dist);
		if( matches[i].distance <= max(2*min_dist, 0.02) )
		{ 
			good_matches.push_back( matches[i]); 
		}
	}
	return (int)good_matches.size();*/
	return 1;
}
