#include "opencv2/core.hpp"
#include "PointPair.h"

using namespace cv;
using namespace std;

class PerspectiveToModelMapper {
public:
	static std::array<PointPair, 3> findNearestThreePointsInModelSpace(Point p, std::vector<PointPair> allPointPairs);
	static void barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w);
};
