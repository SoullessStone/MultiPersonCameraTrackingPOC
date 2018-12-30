#include <PerspectiveToModelMapper.h>

// For Windows...
#include <string>
#include <numeric>
#include <iostream>
#include <vector>
#include <functional>

using namespace std;
// End for Windows...

PerspectiveToModelMapper::PerspectiveToModelMapper()
{
	initBannedConstellations();
	initTriangles();
}

std::array<PointPair, 3> PerspectiveToModelMapper::findNearestThreePointsInModelSpace(
	Point p,				// Point in Perspective, Method tries to find the nearest referencPoints to that point
	std::vector<PointPair> allPointPairs	// Referencepoints to chose from (camera-specific)
)
{
	/*
		Weitere Konfiguration: Dreiecke bestimmen. Wenn Punkt in Perspektive in Dreieck, diese Referenzpunkte nehmen. Sonst halt wie bisher. Das verbessert den Fall Baptiste und Dävu bei Frame 56+
			* check!	init: alle möglichen dreiecke in model erfassen, evtl. allPointPairs in Map abbilden
			* hier: loop über dreiecke (etwa 70)
				* hole punkte, schau ob der (perspektiven-)punkt drin ist
					* wenn ja: auslesen und result setzen, wir berechnen basierend auf diesen punkten
					* wenn nein: wie bisher
					* wenn ein punkt nicht da: wie bisher

	*/
	// Loop over all triangles of the field
	for(PointConstellation& triangle : triangles) {
		// Try to find the points of the triangle (may not be there if out of the view of the camera)
		PointPair pp1 = findPointPairById(triangle.id1, allPointPairs);
		PointPair pp2 = findPointPairById(triangle.id2, allPointPairs);
		PointPair pp3 = findPointPairById(triangle.id3, allPointPairs);
		if (pp1.id == -1 || pp2.id == -1 || pp3.id == -1) {
			// One or more points are out of camera view, cannot use this triangle.
			Logger::log("Tried to find triangle: " + std::to_string(triangle.id1) + ", " + std::to_string(triangle.id2) + ", " + std::to_string(triangle.id3) + ", ", 0);
			Logger::log("Not found the ones with -1: " + std::to_string(pp1.id) + ", " + std::to_string(pp2.id) + ", " + std::to_string(pp3.id) + ", ", 0);
			continue;
		}
		// Check if current point in perspective is inside the current triangle
		if (isPointInTriangle(p, pp1.p1, pp2.p1, pp3.p1) == true) {
			// Yes, use this trinangle to define position of p
			std::array<PointPair, 3> result = {
				pp1, pp2, pp3
			};
			Logger::log("Found a triangle containing the player", 0);
			return result;
		}
	}

	Logger::log("Did not find a triangle containing the player. Doing the old procedure.", 0);
	// Did not find a triangle containing p. Thats why we are looking for other combinations of the reference points
	// based on perspective distance.


	// Init nearest Pointpairs
	PointPair nearestPP1(9999,9999,9999,9999,9999);
	double pp1Distance = 9999.0;
	PointPair nearestPP2(9999,9999,9999,9999,9999);
	double pp2Distance = 9999.0;
	PointPair nearestPP3(99999,999,9999,9999,9999);
	double pp3Distance = 9999.0;

	int x, y;
	for(PointPair& curPP: allPointPairs) {
		x = p.x - curPP.p1.x;
		y = p.y - curPP.p1.y;
		double curDistance = sqrt(pow((double)x, 2.0) + pow((double)y, 2.0));

		// We have to fill every point at least once. That's, what the three if's do
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

		// Compare current point only with pp1, if pp1 is the furthest point
		bool pp1DistanceIsTheBiggest = pp1Distance >= pp2Distance && pp1Distance >= pp3Distance;
		if (pp1DistanceIsTheBiggest && curDistance < pp1Distance) {
			if (! arePointsInLine(curPP, nearestPP2, nearestPP3)) {
				nearestPP1 = curPP;
				pp1Distance = curDistance;
			}
			continue;
		}
		// Compare current point only with pp2, if pp1 is the furthest point
		bool pp2DistanceIsTheBiggest = pp2Distance >= pp1Distance && pp2Distance >= pp3Distance;
		if (pp2DistanceIsTheBiggest && curDistance < pp2Distance) {
			if (! arePointsInLine(nearestPP1, curPP, nearestPP3)) {
				nearestPP2 = curPP;
				pp2Distance = curDistance;
			}
			continue;
		}
		// Compare current point only with pp3, if pp1 is the furthest point
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
			Logger::log("Funny state, look into it...", 2);
		}
	}

	std::array<PointPair, 3> result = {
		nearestPP1, nearestPP2, nearestPP3
	};
	//nearestPP1.print();
	//nearestPP2.print();
	//nearestPP3.print();
	return result;
}

// Try to find a PointPair with the param id in the list allPointPairs
PointPair PerspectiveToModelMapper::findPointPairById(int id, std::vector<PointPair> allPointPairs)
{
	for(PointPair& curPP: allPointPairs) {
		if (curPP.id == id) {
			return curPP;
		}
	}
	return PointPair(-1, -1, -1, -1, -1);
}

// Thanks to: https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
float PerspectiveToModelMapper::sign(Point p1, Point p2, Point p3)
{
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

// Thanks to: https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
// Mathematical explanation: https://math.stackexchange.com/questions/51326/determining-if-an-arbitrary-point-lies-inside-a-triangle-defined-by-three-points
bool PerspectiveToModelMapper::isPointInTriangle(Point pt, Point v1, Point v2, Point v3)
{
	float d1, d2, d3;
	bool has_neg, has_pos;

	// Check on which side of the line pt lies
	d1 = sign(pt, v1, v2);
	d2 = sign(pt, v2, v3);
	d3 = sign(pt, v3, v1);

	has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	// Returns true iff all d-values are less than zero or all d values are more than zero
	// This is the case, iff the point is inside v1, v2 and v3
	return !(has_neg && has_pos);
}

// Thanks to: https://gist.github.com/m1el/53582d6bc952c5bbbec6bf36947400b
// and Real-Time Collision Detection by Christer Ericson -> https://books.google.ch/books?id=4wTNBQAAQBAJ&pg=PA47&lpg=PA47&dq=barycentric(Point+p,+Point+a,+Point+b,+Point+c,+float+%26u,+float+%26v,+float+%26w)&source=bl&ots=Wkbrnlv5nV&sig=B9z84o0BxQH-EFp7bw0GJ0tRV3A&hl=de&sa=X&ved=2ahUKEwjL4oz_2rrfAhWLGCwKHZ02AzgQ6AEwAHoECAkQAQ#v=onepage&q=barycentric(Point%20p%2C%20Point%20a%2C%20Point%20b%2C%20Point%20c%2C%20float%20%26u%2C%20float%20%26v%2C%20float%20%26w)&f=false
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
	// Cramer Rule. This is the lower det
	float denom = d00 * d11 - d01 * d01;
	// Should be v = (d11 * d20 - d10 * d21) / denom;
	// But since the inner_product is symmetric, d01 will do and is already calculated
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
}

bool PerspectiveToModelMapper::arePointsInLine(PointPair a, PointPair b, PointPair c) {
	// If the constellation of the three points is banned, they are "in a line"
	for(PointConstellation& pc : bannedConstellations) {
		if (pc.equals(a.id, b.id, c.id)) {
			//a.print();
			//b.print();
			//c.print();
			return true;
		}
	}
	// More mathematical definition of "in a line"
	return (a.p2.x == b.p2.x && b.p2.x == c.p2.x) || (a.p2.y == b.p2.y && b.p2.y == c.p2.y);
}

void PerspectiveToModelMapper::initTriangles() {
	// Reference point constellations. These are not exactly on a line, but don't work that good
	triangles.push_back(PointConstellation(1, 2, 8));
	triangles.push_back(PointConstellation(2, 8, 9));
	triangles.push_back(PointConstellation(2, 9, 10));
	triangles.push_back(PointConstellation(2, 3, 10));
	triangles.push_back(PointConstellation(3, 10, 11));
	triangles.push_back(PointConstellation(3, 4, 11));
	triangles.push_back(PointConstellation(4, 11, 12));
	triangles.push_back(PointConstellation(4, 5, 12));
	triangles.push_back(PointConstellation(5, 12, 13));
	triangles.push_back(PointConstellation(5, 6, 13));
	triangles.push_back(PointConstellation(6, 13, 14));
	triangles.push_back(PointConstellation(6, 7, 14));
	triangles.push_back(PointConstellation(8, 9, 20));
	triangles.push_back(PointConstellation(9, 20, 21));
	triangles.push_back(PointConstellation(10, 16, 17));
	triangles.push_back(PointConstellation(10, 11, 17));
	triangles.push_back(PointConstellation(11, 17, 18));
	triangles.push_back(PointConstellation(11, 12, 18));
	triangles.push_back(PointConstellation(13, 22, 23));
	triangles.push_back(PointConstellation(13, 14, 23));
	triangles.push_back(PointConstellation(20, 29, 30));
	triangles.push_back(PointConstellation(20, 21, 30));
	triangles.push_back(PointConstellation(25, 31, 32));
	triangles.push_back(PointConstellation(25, 26, 32));
	triangles.push_back(PointConstellation(26, 32, 33));
	triangles.push_back(PointConstellation(26, 27, 33));
	triangles.push_back(PointConstellation(22, 34, 35));
	triangles.push_back(PointConstellation(22, 23, 35));
	triangles.push_back(PointConstellation(29, 36, 37));
	triangles.push_back(PointConstellation(29, 30, 37));
	triangles.push_back(PointConstellation(30, 37, 38));
	triangles.push_back(PointConstellation(30, 31, 38));
	triangles.push_back(PointConstellation(31, 38, 39));
	triangles.push_back(PointConstellation(31, 32, 39));
	triangles.push_back(PointConstellation(32, 39, 40));
	triangles.push_back(PointConstellation(32, 33, 40));
	triangles.push_back(PointConstellation(33, 40, 41));
	triangles.push_back(PointConstellation(33, 34, 41));
	triangles.push_back(PointConstellation(34, 41, 42));
	triangles.push_back(PointConstellation(34, 35, 42));
	triangles.push_back(PointConstellation(15, 43, 44));
	triangles.push_back(PointConstellation(15, 16, 44));
	triangles.push_back(PointConstellation(16, 44, 45));
	triangles.push_back(PointConstellation(16, 17, 45));
	triangles.push_back(PointConstellation(17, 45, 46));
	triangles.push_back(PointConstellation(17, 18, 46));
	triangles.push_back(PointConstellation(18, 46, 47));
	triangles.push_back(PointConstellation(18, 19, 47));
	triangles.push_back(PointConstellation(43, 24, 25));
	triangles.push_back(PointConstellation(43, 44, 25));
	triangles.push_back(PointConstellation(44, 25, 26));
	triangles.push_back(PointConstellation(44, 45, 26));
	triangles.push_back(PointConstellation(45, 26, 27));
	triangles.push_back(PointConstellation(45, 46, 27));
	triangles.push_back(PointConstellation(46, 27, 28));
	triangles.push_back(PointConstellation(46, 47, 28));
	triangles.push_back(PointConstellation(9, 15, 10));
	triangles.push_back(PointConstellation(10, 15, 16));
	triangles.push_back(PointConstellation(9, 15, 21));
	triangles.push_back(PointConstellation(15, 21, 43));
	triangles.push_back(PointConstellation(21, 24, 43));
	triangles.push_back(PointConstellation(21, 24, 30));
	triangles.push_back(PointConstellation(24, 30, 31));
	triangles.push_back(PointConstellation(24, 25, 31));
	triangles.push_back(PointConstellation(12, 18, 19));
	triangles.push_back(PointConstellation(12, 13, 19));
	triangles.push_back(PointConstellation(13, 19, 22));
	triangles.push_back(PointConstellation(19, 47, 22));
	triangles.push_back(PointConstellation(47, 22, 28));
	triangles.push_back(PointConstellation(22, 28, 34));
	triangles.push_back(PointConstellation(27, 28, 33));
	triangles.push_back(PointConstellation(28, 33, 34));
}

void PerspectiveToModelMapper::initBannedConstellations() {
	// Reference point constellations. These are not exactly on a line, but don't work that good
	bannedConstellations.push_back(PointConstellation(1, 9, 15));
	bannedConstellations.push_back(PointConstellation(9, 15, 44));
	bannedConstellations.push_back(PointConstellation(9, 15, 45));
	bannedConstellations.push_back(PointConstellation(9, 15, 16));
	bannedConstellations.push_back(PointConstellation(9, 15, 28));
	bannedConstellations.push_back(PointConstellation(9, 15, 34));
	bannedConstellations.push_back(PointConstellation(9, 15, 42));
	bannedConstellations.push_back(PointConstellation(9, 15, 41));
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
	bannedConstellations.push_back(PointConstellation(27, 15, 9));
	bannedConstellations.push_back(PointConstellation(26, 15, 9));
	bannedConstellations.push_back(PointConstellation(46, 15, 9));
	bannedConstellations.push_back(PointConstellation(25, 31, 43));
}



























