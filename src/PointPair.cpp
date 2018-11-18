#include <PointPair.h>

PointPair::PointPair(int inId, int x1, int y1, int x2, int y2)
{
	id = inId;
	p1 = Point(x1,y1);
	p2 = Point(x2,y2);
}

void PointPair::print() {
	Logger::log(std::to_string(id) + " {" + std::to_string(p1.x) + "/" + std::to_string(p1.y) + " " + std::to_string(p2.x) + "/" + std::to_string(p1.y) + "}", 0);
}
