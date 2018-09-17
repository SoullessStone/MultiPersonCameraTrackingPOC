#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "PointPair.h"

using namespace cv;

class ModelImageGenerator {
public:
	static void createFieldModel(std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue);
};
