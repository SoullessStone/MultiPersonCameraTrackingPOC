#include <PerspectiveToModelMapper.h>

PerspectiveToModelMapper::PerspectiveToModelMapper()
{
	initBannedConstellations();
}

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

		// Initiale Punkte: Jeden Referenzpunkt mal abfüllen
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
			if (! arePointsInLine(nearestPP1, nearestPP2, curPP)) {
				nearestPP3 = curPP;
				pp3Distance = curDistance;
			}
			continue;
		}

		// Aktuellen Punkt nur mit PP1 vergleichen, wenn PP1 auch der am weitesten entfernte ist.
		bool pp1DistanceIsTheBiggest = pp1Distance >= pp2Distance && pp1Distance >= pp3Distance;
		if (pp1DistanceIsTheBiggest && curDistance < pp1Distance) {
			if (! arePointsInLine(curPP, nearestPP2, nearestPP3)) {
				nearestPP1 = curPP;
				pp1Distance = curDistance;
			}
			continue;
		}
		// Aktuellen Punkt nur mit PP2 vergleichen, wenn PP2 auch der am weitesten entfernte ist.
		bool pp2DistanceIsTheBiggest = pp2Distance >= pp1Distance && pp2Distance >= pp3Distance;
		if (pp2DistanceIsTheBiggest && curDistance < pp2Distance) {
			if (! arePointsInLine(nearestPP1, curPP, nearestPP3)) {
				nearestPP2 = curPP;
				pp2Distance = curDistance;
			}
			continue;
		}
		// Aktuellen Punkt nur mit PP3 vergleichen, wenn PP3 auch der am weitesten entfernte ist.
		bool pp3DistanceIsTheBiggest = pp3Distance >= pp2Distance && pp3Distance >= pp1Distance;
		if (pp3DistanceIsTheBiggest && curDistance < pp3Distance) {
			if (! arePointsInLine(nearestPP1, nearestPP2, curPP)) {
				nearestPP3 = curPP;
				pp3Distance = curDistance;
			}
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

bool PerspectiveToModelMapper::arePointsInLine(PointPair a, PointPair b, PointPair c) {
	for(PointConstellation& pc : bannedConstellations) {
		if (pc.equals(a.id, b.id, c.id)) {
			//cout << "killed: " << endl;
			//a.print();
			//b.print();
			//c.print();
			return true;
		}
	}
	return (a.p2.x == b.p2.x && b.p2.x == c.p2.x) || (a.p2.y == b.p2.y && b.p2.y == c.p2.y);
}

void PerspectiveToModelMapper::initBannedConstellations() {
	// Referenzpunktkonstellationen, die zwar nicht exakt auf einer Linie sind und doch schlechte Ergebnisse liefern
	bannedConstellations.push_back(PointConstellation(1, 9, 15));
	bannedConstellations.push_back(PointConstellation(9, 15, 44));
	bannedConstellations.push_back(PointConstellation(9, 15, 45));
	bannedConstellations.push_back(PointConstellation(9, 15, 16));
	bannedConstellations.push_back(PointConstellation(15, 44, 26));
	bannedConstellations.push_back(PointConstellation(44, 26, 40));
	bannedConstellations.push_back(PointConstellation(36, 30, 24));
	bannedConstellations.push_back(PointConstellation(30, 24, 44));
	bannedConstellations.push_back(PointConstellation(24, 44, 17));
	bannedConstellations.push_back(PointConstellation(44, 17, 12));
	bannedConstellations.push_back(PointConstellation(42, 34, 28));
	bannedConstellations.push_back(PointConstellation(34, 28, 46));
	bannedConstellations.push_back(PointConstellation(28, 46, 17));
	bannedConstellations.push_back(PointConstellation(46, 17, 10));
	bannedConstellations.push_back(PointConstellation(7, 13, 19));
	bannedConstellations.push_back(PointConstellation(13, 19, 46));
	bannedConstellations.push_back(PointConstellation(19, 46, 26));
	bannedConstellations.push_back(PointConstellation(46, 26, 31));
	bannedConstellations.push_back(PointConstellation(16, 45, 27));
	bannedConstellations.push_back(PointConstellation(18, 45, 25));
	bannedConstellations.push_back(PointConstellation(25, 32, 40));
	bannedConstellations.push_back(PointConstellation(27, 32, 38));
	bannedConstellations.push_back(PointConstellation(16, 11, 5));
	bannedConstellations.push_back(PointConstellation(3, 11, 18));
	bannedConstellations.push_back(PointConstellation(14, 22, 28));
	bannedConstellations.push_back(PointConstellation(19, 22, 35));
	bannedConstellations.push_back(PointConstellation(15, 21, 29));
	bannedConstellations.push_back(PointConstellation(24, 21, 8));
}



























