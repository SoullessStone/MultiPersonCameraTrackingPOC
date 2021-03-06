#include <iostream>

#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/xfeatures2d.hpp"

#include <Logger.h>

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

class NumberExtractor {
public:
	static int getNumberForPlayer(Mat& player);
private:
	static int getPossibilityForPlayerAndNumber(Mat& player, int number);
	static int countSiftMatches(Mat& player, Mat& number);
};
