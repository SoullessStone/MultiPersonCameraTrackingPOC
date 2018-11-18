// Nötig, weil sonst Klasse doppelt da ist: http://forums.devshed.com/programming-42/compile-error-redefinition-class-437198.html
#ifndef DO_NOT_DEFINE_POINTPAIR_MULTIPLE_TIMES

	#define DO_NOT_DEFINE_POINTPAIR_MULTIPLE_TIMES 

	#include <iostream>
	#include "opencv2/core.hpp"
	#include <Logger.h>

	using namespace std;
	using namespace cv;

	struct PointPair
	{
		Point p1;
		Point p2;
		int id;
		PointPair(int inId, int x1, int y1, int x2, int y2);
		void print();
	};

#endif
