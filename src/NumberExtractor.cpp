#include <NumberExtractor.h>


// TODO Work in progress...
int NumberExtractor::getPossibilityForPlayerAndNumber(Mat& player, int number)
{
	Mat n = imread( "../resources/numbers/"+std::to_string(number)+"_black.jpg", IMREAD_GRAYSCALE );

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

int NumberExtractor::countSiftMatches(Mat& player, Mat& number)
{
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
