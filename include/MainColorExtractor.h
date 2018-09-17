#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>

using namespace cv;

class MainColorExtractor {
public:
	static int getPlayerColor(int, void*, Mat& playerImage);
};
