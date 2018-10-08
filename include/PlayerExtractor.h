#include <iostream>
#include "opencv2/core.hpp"
#include <opencv2/dnn.hpp>
#include <RecognizedPlayer.h>
#include <PointPair.h>
#include <MainColorExtractor.h>
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
	public:
		PlayerExtractor(std::vector<std::string> inClasses, int inConfThreshold);
		std::vector<RecognizedPlayer> extract(Mat& frame, const std::vector<Mat>& outs, Net& net, std::vector<PointPair> referencePoints);
	private:
		void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);
};
