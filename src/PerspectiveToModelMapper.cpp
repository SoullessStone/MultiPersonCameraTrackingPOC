#include <PerspectiveToModelMapper.h>

std::array<PointPair, 3> PerspectiveToModelMapper::findNearestThreePointsInModelSpace(Point p, std::vector<PointPair> allPointPairs)
{
	//cout << "Input p: " << p << endl;

	PointPair nearestPP1(9999,9999,9999,9999,9999);
	double pp1Distance = 9999.0;
	PointPair nearestPP2(9999,9999,9999,9999,9999);
	double pp2Distance = 9999.0;
	PointPair nearestPP3(99999,999,9999,9999,9999);
	double pp3Distance = 9999.0;

	double maxDouble = std::numeric_limits<double>::max();
	int x, y;
	double sum1 = maxDouble, sum2 = maxDouble, sum3 = maxDouble;
	for(PointPair& curPP: allPointPairs) {
		x = p.x - curPP.p1.x;
		y = p.y - curPP.p1.y;
		double curDistance = sqrt(pow((double)x, 2.0) + pow((double)y, 2.0));
		//cout << "curPP: #" << curPP.id << " " << curPP.p1 << ", distance: " << curDistance << endl;

		if (pp1Distance == 9999.0) {
			nearestPP1 = curPP;
			pp1Distance = curDistance;
			continue;
		}
		if (pp2Distance == 9999.0) {
			nearestPP2 = curPP;
			pp2Distance = curDistance;
			continue;
		}
		if (pp3Distance == 9999.0) {
			nearestPP3 = curPP;
			pp3Distance = curDistance;
			continue;
		}
		bool pp1DistanceIsTheBiggest = pp1Distance >= pp2Distance && pp1Distance >= pp3Distance;
		if (pp1DistanceIsTheBiggest && curDistance < pp1Distance) {
			nearestPP1 = curPP;
			pp1Distance = curDistance;
			continue;
		}
		bool pp2DistanceIsTheBiggest = pp2Distance >= pp1Distance && pp2Distance >= pp3Distance;
		if (pp2DistanceIsTheBiggest && curDistance < pp2Distance) {
			nearestPP2 = curPP;
			pp2Distance = curDistance;
			continue;
		}
		bool pp3DistanceIsTheBiggest = pp3Distance >= pp2Distance && pp3Distance >= pp1Distance;
		if (pp3DistanceIsTheBiggest && curDistance < pp3Distance) {
			nearestPP3 = curPP;
			pp3Distance = curDistance;
			continue;
		}
		if (!(pp1DistanceIsTheBiggest||pp2DistanceIsTheBiggest||pp3DistanceIsTheBiggest))
		{
			cout << "Komischer State, mal reinschauen..." << endl;
		}
	}

	std::array<PointPair, 3> result = {
		nearestPP1, nearestPP2, nearestPP3
	};
	return result;
}

void PerspectiveToModelMapper::barycentric(Point p, Point a, Point b, Point c, float &u, float &v, float &w)
{
	int v0[] = { b.x-a.x, b.y-a.y };
  	int v1[] = { c.x-a.x, c.y-a.y };
  	int v2[] = { p.x-a.x, p.y-a.y };
	int array_size = 2;
	float d00 = inner_product(v0, v0 + array_size, v0, 0);
	float d01 = inner_product(v0, v0 + array_size, v1, 0);
	float d11 = inner_product(v1, v1 + array_size, v1, 0);
	float d20 = inner_product(v2, v2 + array_size, v0, 0);
	float d21 = inner_product(v2, v2 + array_size, v1, 0);
	float denom = d00 * d11 - d01 * d01;
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
}
