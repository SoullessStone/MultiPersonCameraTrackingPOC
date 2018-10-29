#include <TrackingModule.h>

void TrackingModule::handleInput(int frameId, std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// Try to merge players from the three cameras
	std::vector<RecognizedPlayer> curFrameInput = getMergedInput(inputHud, inputMar, inputMic);

	std::vector<RecognizedPlayer> newHistoryInput;

	if(history.find(frameId - 1) == history.end())
	{
		// First frame, no help from history
		history.insert(std::make_pair(frameId, curFrameInput));
	} else {
		for (RecognizedPlayer& histPlayer : history.find(frameId - 1)->second) {
			// Try to find the previously known players
			bool hasMatched = false;
			cout << "History-Player #" << histPlayer.getCamerasPlayerId() << endl;
			auto curFramePlayer = std::begin(curFrameInput);
			while (curFramePlayer != std::end(curFrameInput)) {
				cout << "+++++++++++++++++ Trying to match #" << (*curFramePlayer).getCamerasPlayerId() << endl;
				if (isPossiblySamePlayer(histPlayer, *curFramePlayer, 300)) {
					// We found the player again
					cout << "+++++++++++++++++ wow, WOW, WOOOOOOOOOOOW!!!!" << endl;
					// TODO evtl. den besten match finden
					(*curFramePlayer).setCamerasPlayerId(histPlayer.getCamerasPlayerId());
					newHistoryInput.push_back(*curFramePlayer);
					curFramePlayer = curFrameInput.erase(curFramePlayer);
					hasMatched = true;
					break;
				}else {
					cout << "+++++++++++++++++ nope" << endl;
					++curFramePlayer;
				}
			}
			if (! hasMatched) {
				// Histplayer was not found in new input. We fill the gap temporarly

				// TODO: Nur einige Frames behalten, amsonsten leben Geister weiter bei uns
				newHistoryInput.push_back(histPlayer);
			}
		}
		history.insert(std::make_pair(frameId, newHistoryInput));
	}

	// Print them (Debug)
	printHistory();
}

std::vector<RecognizedPlayer> TrackingModule::getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	std::vector<RecognizedPlayer> mergedPlayers;
	std::vector<PointPair> playersToDraw;
	int threshold = 200;

	RecognizedPlayer rp2;
	// Input: Confidence: Wenn alle Kameras sich einig sind: hoch, sonst tief. Anfang tief, Vergangenheit nicht viel einbeziehen
	cout << "inputMic.size() " << inputMic.size() << endl;
	for (RecognizedPlayer& rp : inputMic) {
		// TODO: Mit Abstand kleinest diff => same! auch wenn über 200px
		cout << "inputMic " << rp.getCamerasPlayerId() << endl;
		RecognizedPlayer resultingPlayerA;
		bool validPlayerA = false;
		RecognizedPlayer resultingPlayerB;
		bool validPlayerB = false;

		auto i = std::begin(inputHud);

		while (i != std::end(inputHud)) {
			cout << "a-------- " << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				cout << "a-------- " << "same" << endl;	
				resultingPlayerA = *i;
				validPlayerA = true;
				i = inputHud.erase(i);
			}else {
				//cout << "a-------- " << "different" << endl;
				++i;
			}
		}

		i = std::begin(inputMar);

		while (i != std::end(inputMar)) {
			cout << "b-------- " << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				cout << "b-------- " << "same" << endl;	
				resultingPlayerB = *i;
				validPlayerB = true;
				i = inputMar.erase(i);
			}else {
				//cout << "b-------- " << "different" << endl;
				++i;
			}
		}
		if (validPlayerA && !validPlayerB) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerA.getPositionInModel().x) / 2;
			int y = (rp.getPositionInModel().y+resultingPlayerA.getPositionInModel().y) / 2;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);
			playersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));
		}
		if (!validPlayerA && validPlayerB) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerB.getPositionInModel().x) / 2;
			int y = (rp.getPositionInModel().y+resultingPlayerB.getPositionInModel().y) / 2;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);
			playersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));
			
		}
		if (validPlayerA && validPlayerB) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerA.getPositionInModel().x+resultingPlayerB.getPositionInModel().x) / 3;
			int y = (rp.getPositionInModel().y+resultingPlayerA.getPositionInModel().y+resultingPlayerB.getPositionInModel().y) / 3;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);	
			playersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));		
		}
	}
	cout << "inputHud.size() " << inputHud.size() << endl;
	for (RecognizedPlayer& rp : inputHud) {
		cout << "inputHud " << rp.getCamerasPlayerId() << endl;

		auto i = std::begin(inputMar);

		while (i != std::end(inputMar)) {
			cout << "c-------- " << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				cout << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc-------- " << "same" << endl;	
				RecognizedPlayer curPlayer;
				curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 300);
				curPlayer.setIsRed(rp.getIsRed(), true);
				int x = (rp.getPositionInModel().x+(*i).getPositionInModel().x) / 2;
				int y = (rp.getPositionInModel().y+(*i).getPositionInModel().y) / 2;
				curPlayer.setPositionInModel(Point(x, y), true);
				mergedPlayers.push_back(curPlayer);
				playersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 300, -1, -1, x, y));
				i = inputMar.erase(i);
			}else {
				//cout << "c-------- " << "different" << endl;
				++i;
			}
		}

	}
	cout << "inputHud.size() " << inputHud.size() << endl;

	std::vector<PointPair> referencePoints;
	std::vector<PointPair> linesToDraw;
	ModelImageGenerator::createFieldModel(referencePoints, linesToDraw, playersToDraw);
	
	return mergedPlayers;
}

void TrackingModule::printHistory()
{
	std::map<int, std::vector<RecognizedPlayer>>::iterator it = history.begin();
	while(it != history.end())
	{
		std::cout<<it->first<<" :: size -> "<<it->second.size() << std::endl;
		//for(RecognizedPlayer& player : it->second) {
		//	cout << player.toString() << endl;
		//}
		it++;
	}
}

bool TrackingModule::isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold)
{
	int diffX = a.getPositionInModel().x - b.getPositionInModel().x;
	int diffY = a.getPositionInModel().y - b.getPositionInModel().y;
	if (diffX < 0)
		diffX = diffX * -1;
	if (diffY < 0)
		diffY = diffY * -1;
	//cout << "--------- " << "diff: " << diffX + diffY << endl;
	if (diffX + diffY < threshold) {
		if (a.getIsRed() == b.getIsRed()) {
			return true;
		}
	}
	return false;
}
