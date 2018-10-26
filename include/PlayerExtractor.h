#include <iostream>
#include "opencv2/core.hpp"
#include <opencv2/dnn.hpp>
#include <RecognizedPlayer.h>
#include <PointPair.h>
#include <MainColorExtractor.h>
#include <NumberExtractor.h>
#include <PerspectiveToModelMapper.h>
#include <PointPair.h>
#include <ModelImageGenerator.h>

using namespace std;
using namespace cv;
using namespace dnn;

class PlayerExtractor {
	PerspectiveToModelMapper perspectiveToModelMapper;
	std::vector<std::string> classes;
	int confThreshold;
	Net net;
	float scale;
	Scalar mean;
	bool swapRB;
	int inpWidth;
	int inpHeight;
	Mat tempBlob;
	public:
		PlayerExtractor(std::vector<std::string> classes, int confThreshold, Net& net, float scale, Scalar mean, bool swapRB, int inpWidth, int inpHeight);
		std::vector<RecognizedPlayer> extract(Mat& frame, const std::vector<Mat>& outs, std::vector<PointPair> referencePoints);
		std::vector<Mat> getOuts(Mat frame);
	private:
		std::vector<String> getOutputsNames(const Net& net);
};
