#include <PointConstellation.h>

PointConstellation::PointConstellation(int id1, int id2, int id3)
{
	this->id1 = id1;
	this->id2 = id2;
	this->id3 = id3;
}

void PointConstellation::print() {
	cout << " {" << id1 << " " << id2 << " " << id3 << "}" << endl;
}

bool PointConstellation::equals(int a, int b, int c) {
	return ((a == id1) && (b == id2) && (c == id3))
		|| ((a == id1) && (c == id2) && (b == id3))
		|| ((b == id1) && (a == id2) && (c == id3))
		|| ((b == id1) && (c == id2) && (a == id3))
		|| ((c == id1) && (a == id2) && (b == id3))
		|| ((c == id1) && (b == id2) && (a == id3));
}
