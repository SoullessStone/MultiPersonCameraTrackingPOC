#include "opencv2/core.hpp"
#include "Logger.h"
#include "PointPair.h"
#include <PointConstellation.h>

using namespace cv;
using namespace std;

class PerspectiveToModelMapper {

public:
	std::array<PointPair, 3> findNearestThreePointsInModelSpace(Point p, std::vector<PointPair> allPointPairs);
	void barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w);
	PerspectiveToModelMapper();
private:
	bool arePointsInLine(PointPair a, PointPair b, PointPair c);
	void initBannedConstellations();
	void initTriangles();
	PointPair findPointPairById(int id, std::vector<PointPair> allPointPairs);
	// Both methods from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
	float sign(Point p1, Point p2, Point p3);
	bool isPointInTriangle(Point pt, Point v1, Point v2, Point v3);

	std::vector<PointConstellation> bannedConstellations;
	std::vector<PointConstellation> triangles;
};
