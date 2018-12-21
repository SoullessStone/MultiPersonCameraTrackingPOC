#include <Camera.h>

Camera::Camera(int inId, const std::string& inFile, int startPoint)
{
	id = inId;
	cap = VideoCapture();
	cap.open(inFile);
	wantedMs = startPoint;
}

int Camera::getWantedMs() {
	return wantedMs;
}

double Camera::getLastUsedMs() {
	return lastUsedMs;
}

Mat Camera::getNextFrame()
{
	// waiting for right frame
	for (;;) {
		if (maxMs < wantedMs) {
			// Video ends here
			throw -1;
		}
		int upperThreshold = (int)cap.get(CV_CAP_PROP_POS_MSEC) + 20;
		int lowerThreshold = (int)cap.get(CV_CAP_PROP_POS_MSEC) - 20;
		cap >> frame;
		if  (wantedMs < upperThreshold && wantedMs > lowerThreshold) {
			break;
		}
	}
	// Define the time to wait
	wantedMs += 100;
	//cout << "Camera #" << id << ": return frame at " << cap.get(CV_CAP_PROP_POS_MSEC ) << "ms" << endl;
	//cout << "Camera #" << id << " ratio: " << cap.get(CAP_PROP_POS_AVI_RATIO  ) << endl;
	//cout << "Camera #" << id << " frame " << cap.get(CAP_PROP_POS_FRAMES   ) << endl;
	//cout << "Camera #" << id << " framecount: " << cap.get(CAP_PROP_FRAME_COUNT    ) << endl;
	lastUsedMs = cap.get(CV_CAP_PROP_POS_MSEC);
	return frame;
}
