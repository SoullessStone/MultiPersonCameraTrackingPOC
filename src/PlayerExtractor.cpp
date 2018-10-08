#include <PlayerExtractor.h>


PlayerExtractor::PlayerExtractor(std::vector<std::string> inClasses, int inConfThreshold) {
	classes = inClasses;
	confThreshold = inConfThreshold;
}

std::vector<RecognizedPlayer> PlayerExtractor::extract(Mat& frame, const std::vector<Mat>& outs, Net& net, std::vector<PointPair> referencePoints)
{
	std::vector<RecognizedPlayer> returnablePlayers;
	returnablePlayers.push_back(RecognizedPlayer());
	static std::vector<int> outLayers = net.getUnconnectedOutLayers();
	static std::string outLayerType = net.getLayer(outLayers[0])->type;

	if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
	{
		cout << "im_info" << endl;
	}
	else if (outLayerType == "DetectionOutput")
	{
		cout << "DetectionOutput" << endl;
	}
	else if (outLayerType == "Region")
	{
		std::vector<int> classIds;
		std::vector<float> confidences;
		std::vector<Rect> boxes;
		int counter = 0;
		std::vector<PointPair> linesToDraw;
		std::vector<PointPair> playersToDraw;
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
					int centerX = (int)(data[0] * frame.cols);
					int centerY = (int)(data[1] * frame.rows);
					int width = (int)(data[2] * frame.cols);
					int height = (int)(data[3] * frame.rows);
					int left = centerX - width / 2;
					int top = centerY - height / 2;
					int bottom = centerY + height / 2;
					// Handle Players
					if (classIdPoint.x == 0) {
						counter += 1;
						// Extract player from frame
						Mat player;
						player = frame(Rect(left, top, width, height));
						int playerNumber = counter;
						// Check if red or black player
						if (MainColorExtractor::getPlayerColor(0, 0, player) == 1) {
							playerNumber += 100;
							//cout << "Red Player" << endl;

							// Find number
							// TODO Etwas finden, das funktioniert
							/*cv::Mat greyPlayer;
							cv::cvtColor(player, greyPlayer, cv::COLOR_BGR2GRAY);
							std::array<int, 7> possibleNumbers = {1,3,4,5,6,8,9};
							for(int& i: possibleNumbers) { 
								cout << "------ Possibility for " << i << ": " << NumberExtractor::getPossibilityForPlayerAndNumber(greyPlayer, i) << endl;
								//waitKey();
							}*/
						}
						else {
							//cout << "Black Player" << endl;
						}

						// Unterster Punkt des Spielers erkennen
						Point bottomOfPlayer(centerX, bottom);

						putText(frame, std::to_string(playerNumber), bottomOfPlayer, FONT_HERSHEY_COMPLEX_SMALL, 1.5, cvScalar(0,200,250), 1, CV_AA);
						// In der Perspektive die nähesten drei Keypoints finden

						std::array<PointPair, 3> nearestPoints = perspectiveToModelMapper.findNearestThreePointsInModelSpace(bottomOfPlayer, referencePoints);
						
						// bottomOfPlayer als baryzentrische Koordinaten in Bezug auf die drei nähesten Punkte beschreiben
						float u = 0.0;
						float v = 0.0;
						float w = 0.0;
						perspectiveToModelMapper.barycentric(bottomOfPlayer, nearestPoints[0].p1, nearestPoints[1].p1, nearestPoints[2].p1, u, v, w);
						
						// Position des Spielers in Modelkoordinaten ausdrücken
						float x_part = u*(float)nearestPoints[0].p2.x + v*(float)nearestPoints[1].p2.x + w*(float)nearestPoints[2].p2.x;
						float y_part = u*(float)nearestPoints[0].p2.y + v*(float)nearestPoints[1].p2.y + w*(float)nearestPoints[2].p2.y;

						if (x_part < 0) {
							x_part = 0;
						}
						if (y_part < 0) {
							y_part = 0;
						}

						PointPair playerPointPair(playerNumber, bottomOfPlayer.x, bottomOfPlayer.y, x_part, y_part);
						playersToDraw.push_back(playerPointPair);

						linesToDraw.push_back(PointPair(0, nearestPoints[0].p2.x, nearestPoints[0].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[1].p2.x, nearestPoints[1].p2.y, x_part, y_part));
						linesToDraw.push_back(PointPair(0, nearestPoints[2].p2.x, nearestPoints[2].p2.y, x_part, y_part));
					}

					classIds.push_back(classIdPoint.x);
					confidences.push_back((float)confidence);
					boxes.push_back(Rect(left, top, width, height));
				}
			}
		}
		ModelImageGenerator::createFieldModel(referencePoints, linesToDraw, playersToDraw);

		for(PointPair& pp: referencePoints) {
			circle(frame, Point(pp.p1.x, pp.p1.y), 8, Scalar(0, 0, 255));
			putText(frame, std::to_string(pp.id), cvPoint(pp.p1.x+15,pp.p1.y+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
		}
		std::vector<int> indices;
		NMSBoxes(boxes, confidences, confThreshold, 0.4, indices);
		for (size_t i = 0; i < indices.size(); ++i)
		{
			int idx = indices[i];
			Rect box = boxes[idx];
			drawPred(classIds[idx], confidences[idx], box.x, box.y,
				box.x + box.width, box.y + box.height, frame);
		}
	}
	else
		CV_Error(Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);
	return returnablePlayers;
}

void PlayerExtractor::drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
	rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

	std::string label = format("%.2f", conf);
	if (!classes.empty())
	{
		CV_Assert(classId < (int)classes.size());
		label = classes[classId] + ": " + label;
	}

	int baseLine;
	Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

	top = max(top, labelSize.height);
	rectangle(frame, Point(left, top - labelSize.height),
		Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
	putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
}
