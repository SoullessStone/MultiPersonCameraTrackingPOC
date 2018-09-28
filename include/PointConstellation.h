
#include <iostream>

using namespace std;

struct PointConstellation
{
	int id1;
	int id2;
	int id3;
	PointConstellation(int a, int b, int c);
	void print();
	bool equals(int a, int b, int c);
};
