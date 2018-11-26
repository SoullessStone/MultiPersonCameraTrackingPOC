// Nötig, weil sonst Klasse doppelt da ist: http://forums.devshed.com/programming-42/compile-error-redefinition-class-437198.html
#ifndef DO_NOT_DEFINE_MODELIMAGEGENERATOR_MULTIPLE_TIMES

	#define DO_NOT_DEFINE_MODELIMAGEGENERATOR_MULTIPLE_TIMES 

	#include "opencv2/core.hpp"
	#include <opencv2/imgproc.hpp>
	#include <opencv2/highgui.hpp>
	#include "PointPair.h"

	using namespace cv;

	class ModelImageGenerator {
	public:
		static void createFieldModel(std::string title, std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue);
		static void createFieldModel(std::string title, std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue, std::vector<PointPair> additionalPointsYellow);
		static void createFieldModel(std::string title, std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlue, std::vector<PointPair> additionalPointsYellow, std::vector<PointPair> basetruth);
	};

#endif
