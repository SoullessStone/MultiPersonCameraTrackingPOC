#include <PointPair.h>

PointPair::PointPair(int inId, int x1, int y1, int x2, int y2)
{
	id = inId;
	p1 = Point(x1,y1);
	p2 = Point(x2,y2);
}

void PointPair::print() {
	cout << id << " {" << p1 << " " << p2 << "}" << endl;
}
