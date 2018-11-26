// Nötig, weil sonst Klasse doppelt da ist: http://forums.devshed.com/programming-42/compile-error-redefinition-class-437198.html
#ifndef DO_NOT_DEFINE_MAINCOLOREXTRACTOR_MULTIPLE_TIMES

	#define DO_NOT_DEFINE_MAINCOLOREXTRACTOR_MULTIPLE_TIMES 

	#include "opencv2/core.hpp"
	#include <opencv2/imgproc.hpp>

	using namespace cv;

	class MainColorExtractor {
	public:
		static int getPlayerColor(int, void*, Mat& playerImage, bool normalFrame);
	};

#endif
