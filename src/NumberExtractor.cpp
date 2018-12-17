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
	return 1;
}
