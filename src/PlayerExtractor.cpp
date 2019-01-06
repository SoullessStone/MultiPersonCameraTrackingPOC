#include <PlayerExtractor.h>

struct DrawObject
{
	int playerNumber;
	Point textPosition;
	Point rectanglePointA;
	Point rectanglePointB;
	bool isTypeA;

	DrawObject(int number, Point a, Point b, Point c, bool type)
	{
		playerNumber = number;
		textPosition = a;
		rectanglePointA = b;
		rectanglePointB = c;
		isTypeA = type;
	}
};

PlayerExtractor::PlayerExtractor(std::vector<std::string> classes, int confThreshold, Net& net, float scale, Scalar mean, bool swapRB, int inpWidth, int inpHeight) {
	this->classes = classes;
	this->confThreshold = confThreshold;
	this->net = net;
	this->scale = scale;
	this->mean = mean;
	this->swapRB = swapRB;
	this->inpWidth = inpWidth;
	this->inpHeight = inpHeight;
}

// As seen here: https://github.com/opencv/opencv/blob/3.4/samples/dnn/object_detection.cpp
// Prepares the input frame and gets the results from DNN
std::vector<Mat> PlayerExtractor::getOuts(Mat frame) {
		// Create a 4D blob from a frame.
		Size inpSize(inpWidth > 0 ? inpWidth : frame.cols,
			inpHeight > 0 ? inpHeight : frame.rows);
		blobFromImage(frame, tempBlob, scale, inpSize, mean, swapRB, false);

		// Run a model.
		net.setInput(tempBlob);
		if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
		{
			resize(frame, frame, inpSize);
			Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
			net.setInput(imInfo, "im_info");
		}
		std::vector<Mat> outs;
		net.forward(outs, getOutputsNames(net));
		return outs;
}

std::vector<String> PlayerExtractor::getOutputsNames(const Net& net)
{
	static std::vector<String> names;
	if (names.empty())
	{
		std::vector<int> outLayers = net.getUnconnectedOutLayers();
		std::vector<String> layersNames = net.getLayerNames();
		names.resize(outLayers.size());
		for (size_t i = 0; i < outLayers.size(); ++i)
			names[i] = layersNames[outLayers[i] - 1];
	}
	return names;
}

std::vector<RecognizedPlayer> PlayerExtractor::extract(
	Mat& frame,					// Frame to work with
	const std::vector<Mat>& outs,			// DNN-Results for this frame
	std::vector<PointPair> referencePoints,		// Referencepoints for the camera that produced the frame
	int sizeThreshold,				// Filter players smaller than the threshold
	bool isNormalFrame				// <Deprecated> Flag to chose between color-detection-algorithms
)
{
	std::vector<RecognizedPlayer> returnablePlayers;
	static std::vector<int> outLayers = net.getUnconnectedOutLayers();
	static std::string outLayerType = net.getLayer(outLayers[0])->type;

	if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
	{
		Logger::log("im_info", 0);
	}
	else if (outLayerType == "DetectionOutput")
	{
		Logger::log("DetectionOutput", 0);
	}
	else if (outLayerType == "Region")
	{
		int counter = 0;
		std::vector<PointPair> linesToDraw;
		std::vector<PointPair> playersToDraw;
		std::vector<DrawObject> drawObjectsFrame;
		for (size_t i = 0; i < outs.size(); ++i)
		{
			// Network produces output blob with a shape NxC where N is a number of
			// detected objects and C is a number of classes + 4 where the first 4
			// numbers are [center_x, center_y, width, height]
			float* data = (float*)outs[i].data;
			for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
			{
				Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
				Point classIdPoint;
				double confidence;
				minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
				if (confidence > confThreshold)
				{					
					// Handle Players (Label==0 stands for Person)
					if (classIdPoint.x == 0) {
						// Get information about detected person
						int centerX = (int)(data[0] * frame.cols);
						int centerY = (int)(data[1] * frame.rows);
						int width = (int)(data[2] * frame.cols);
						int height = (int)(data[3] * frame.rows);
						int left = centerX - width / 2;
						int top = centerY - height / 2;
						int bottom = centerY + height / 2;

						// Skip detection, if too small
						if (width*height < sizeThreshold) {
							Logger::log("Skipped because the patch is too small.", 0);
							continue;
						}

						// Sometimes the result is out of bounds, we cannot allow that.
						if(left + width >= frame.cols) {
							Logger::log("Trim player (width)", 0);
							width = frame.cols - left - 1;
						}
						if(top + height >= frame.rows) {
							Logger::log("Trim player (height)", 0);
							height = frame.rows - top - 1;
						}
						// Sometimes the start point is also out of bounds, we have to fix that also
						if (left < 0) {
							Logger::log("Trim player (left)", 0);
							left = 0;
						}
						if (top < 0) {
							Logger::log("Trim player (top)", 0);
							top = 0;
						}

						// Create container in which we store the information about the player
						RecognizedPlayer currentPlayer;

						counter += 1;
						Mat player = frame(Rect(left, top, width, height));
						
						int playerNumber = counter;
						// Check if red or yellow player
						if (MainColorExtractor::getPlayerColor(0, 0, player, isNormalFrame) == 1) {
							// Add 100 to the playerNumber. The color is better distinguishable
							playerNumber += 100;

							// Removed Numberdetection because it is slow and not worth the time
							/* Find number
							Mat greyPlayer;
							cv::cvtColor(player, greyPlayer, cv::COLOR_BGR2GRAY);
							int result = NumberExtractor::getNumberForPlayer(greyPlayer);
							if (result == -1){
								currentPlayer.setShirtNumber(-1, false);
							} else {
								currentPlayer.setShirtNumber(result, true);
								//Loggercout << result << endl;
								imshow("player", player);
								waitKey();
							}*/

							currentPlayer.setIsRed(true, true);
						}
						else {
							currentPlayer.setShirtNumber(-1, false);
							currentPlayer.setIsRed(false, true);
						}

						currentPlayer.setCamerasPlayerId(playerNumber);
						//imwrite("players/" + std::to_string(counterForExport+5000) + "_" + std::to_string(playerNumber) + ".jpg", player);
						counterForExport++;

						// Find the bottom part of the player
						Point bottomOfPlayer(centerX, bottom);
						currentPlayer.setPositionInPerspective(bottomOfPlayer);

						Logger::log("-------------- Player #" + std::to_string(playerNumber), 0);
						// Find the three nearest PointPairs in perspective
						std::array<PointPair, 3> nearestPoints = perspectiveToModelMapper.findNearestThreePointsInModelSpace(bottomOfPlayer, referencePoints);
						nearestPoints[0].print();
						nearestPoints[1].print();
						nearestPoints[2].print();
						
						// Describe bottomOfPlayer as baryzentric coordinates in relation to the three nearest points
						float u = 0.0;
						float v = 0.0;
						float w = 0.0;
						perspectiveToModelMapper.barycentric(bottomOfPlayer, nearestPoints[0].p1, nearestPoints[1].p1, nearestPoints[2].p1, u, v, w);
						
						// Calculate position of player in model coordinates
						float x_part = u*(float)nearestPoints[0].p2.x + v*(float)nearestPoints[1].p2.x + w*(float)nearestPoints[2].p2.x;
						float y_part = u*(float)nearestPoints[0].p2.y + v*(float)nearestPoints[1].p2.y + w*(float)nearestPoints[2].p2.y;

						// Could be out of bounds, we don't want that
						if (x_part < 0) {
							x_part = 0;
						}
						if (y_part < 0) {
							y_part = 0;
						}
						currentPlayer.setPositionInModel(Point(x_part, y_part), true);

						// Add some things to be drawn later
						PointPair playerPointPair(playerNumber, bottomOfPlayer.x, bottomOfPlayer.y, x_part, y_part);

						linesToDraw.push_back(PointPair(0, nearestPoints[0].p2.x, nearestPoints[0].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[1].p2.x, nearestPoints[1].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[2].p2.x, nearestPoints[2].p2.y, x_part, y_part));
						
						// Sometimes, the NN detects a person as two. So we delete the copy
						bool isDuplicate = false;
						for(RecognizedPlayer& rPlayer: returnablePlayers) {
							Point rPlayerPerspectivePosition = rPlayer.getPositionInPerspective();
							int diffX = rPlayerPerspectivePosition.x - bottomOfPlayer.x;
							int diffY = rPlayerPerspectivePosition.y - bottomOfPlayer.y;
							if (diffX < 0)
								diffX = diffX * -1;
							if (diffY < 0)
								diffY = diffY * -1;
							if (diffX + diffY < 30 && rPlayer.getIsRed() == currentPlayer.getIsRed()) {
								isDuplicate = true;
								Logger::log("Skipped dublicate - diffX/diffY: " + std::to_string(diffX) + "/" + std::to_string(diffY), 0);
								Logger::log("currentId/alreadyId: " + std::to_string(currentPlayer.getCamerasPlayerId()) + "/" + std::to_string(rPlayer.getCamerasPlayerId()), 0);
								break;
							}
						}
						if (! isDuplicate) {
							// If it not a duplicate, we add it to the resulting list
							returnablePlayers.push_back(currentPlayer);
							playersToDraw.push_back(playerPointPair);
							// Draw the players number near him
							drawObjectsFrame.push_back(DrawObject(playerNumber, bottomOfPlayer, Point(left, top), Point(left + width, bottom), true));
						} else {
							drawObjectsFrame.push_back(DrawObject(playerNumber, bottomOfPlayer, Point(left, top), Point(left + width, bottom), false));
						}
					}
				}
			}
		}
		// Draw to frame
		for (DrawObject& drawObj : drawObjectsFrame) {
			cv::Scalar textColor;
			cv::Scalar rectColor;
			if (drawObj.isTypeA == true) {
				textColor = cvScalar(255,255,255);
				rectColor = Scalar(0, 255, 0);
			} else {
				textColor = cvScalar(0,255,255);
				rectColor = Scalar(0, 0, 255);
			}
			putText(frame, std::to_string(drawObj.playerNumber), drawObj.textPosition, FONT_HERSHEY_COMPLEX_SMALL, 2, textColor, 2, CV_AA);
			rectangle(frame, drawObj.rectanglePointA, drawObj.rectanglePointB, rectColor);
		}

		// Create the image of the model with some additional information (players, used reference points etc)
		if (referencePoints.size() == 37)
			ModelImageGenerator::createFieldModel("Extraction from image Hud", referencePoints, linesToDraw, playersToDraw);
		else if (referencePoints.size() == 28)
			ModelImageGenerator::createFieldModel("Extraction from image Mar", referencePoints, linesToDraw, playersToDraw);
		else
			ModelImageGenerator::createFieldModel("Extraction from image Mic", referencePoints, linesToDraw, playersToDraw);

		// Draw some things on the frame
		for(PointPair& pp: referencePoints) {
			circle(frame, Point(pp.p1.x, pp.p1.y), 8, Scalar(0, 0, 255));
			putText(frame, std::to_string(pp.id), cvPoint(pp.p1.x+15,pp.p1.y+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
		}
	}
	else
		CV_Error(Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);

	return returnablePlayers;
}



