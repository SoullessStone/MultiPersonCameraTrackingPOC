#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>

using namespace cv;

// TODO: evtl. noch obere dinge aus Main entfernen

class MainColorExtractor {
public:
	static int getPlayerColor(int, void*, Mat& playerImage);
};
