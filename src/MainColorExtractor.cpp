#include <MainColorExtractor.h>

// TODO: Eleganter lösen (frei wählbare Farben für beide Teams, count und vergleichen, evtl hsv-distance: http://answers.opencv.org/question/127885/how-can-i-best-compare-two-bgr-colors-to-determine-how-similardifferent-they-are/)
int MainColorExtractor::getPlayerColor(int, void*, Mat& playerImage)
{
	// Convert input image to HSV
	cv::Mat hsv_image;
	cv::cvtColor(playerImage, hsv_image, cv::COLOR_BGR2HSV);

	// Normalizing through standarizing the image size
	resize(hsv_image, hsv_image, Size(100, 100));

	// Threshold the HSV image, keep only the red pixels 
	cv::Mat lower_red_hue_range;
	cv::Mat upper_red_hue_range;
	cv::inRange(hsv_image, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), lower_red_hue_range);
	cv::inRange(hsv_image, cv::Scalar(160, 100, 100), cv::Scalar(179, 255, 255), upper_red_hue_range);
	int countA = countNonZero(lower_red_hue_range);
	int countB = countNonZero(upper_red_hue_range);
	float percent = (float)(((float)countA+(float)countB) / ((float) 2*(float)hsv_image.rows*(float)hsv_image.cols));

	// Magic number :)
	return percent > 0.025;
}
