#include <iostream>
#include "opencv2/core.hpp"

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
