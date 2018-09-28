#include "opencv2/core.hpp"
#include "PointPair.h"
#include <PointConstellation.h>

using namespace cv;
using namespace std;

class PerspectiveToModelMapper {

public:
	std::vector<PointConstellation> bannedConstellations;
	std::array<PointPair, 3> findNearestThreePointsInModelSpace(Point p, std::vector<PointPair> allPointPairs);
	void barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w);
private:
	bool arePointsInLine(PointPair a, PointPair b, PointPair c);
	void initBannedConstellations();
};
