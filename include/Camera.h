#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

struct Camera
{
	int id;
	VideoCapture cap;
	Mat frame;
	int wantedMs;
	int maxMs = 70140;
	double lastUsedMs = 0;
	int firstFrame = true;

	Camera(int inId, const std::string& inFile, int startPoint);

	int getWantedMs();

	double getLastUsedMs();

	Mat getNextFrame();
};
