#include <iostream>

#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/xfeatures2d.hpp"

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

class NumberExtractor {
public:
	static int getPossibilityForPlayerAndNumber(Mat& player, int number);
private:
	static int countSiftMatches(Mat& player, Mat& number);
};
