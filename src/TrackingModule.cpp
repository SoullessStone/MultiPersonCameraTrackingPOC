#include <TrackingModule.h>

// TODO Verbesserungsideen
// 3 Dinge: Löschen/Neu erstellen, Distanz minimieren, Merging anpassen/weniger restriktiv
	//  o Ideale Zuteilung von history-Spielern und input herausfinden (globales minimieren von distanz, schwarz/rot trennen)
	//  o Evtl. weniger mergen/ mehr Input zulassen.
	//     x für kameras Gebiete der absoluten Macht definieren. Wenn camera1 meint, in dem bereich steht ein Spieler, dann steht da einer
	//  o Neuerfassung (nicht nur am Anfang) ermöglichen!
	//  o Lost Player durchdenken
	//     x wiederfinden evtl. auch mit shirt-nummer?

void TrackingModule::handleInput(int frameId, std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// Try to merge players from the three cameras
	std::vector<RecognizedPlayer> curFrameInput = getMergedInput(inputHud, inputMar, inputMic);

	// New input to be added to the "memory"
	std::vector<RecognizedPlayer> newHistoryInput;
	// Debug: Points to print in debug-image
	std::vector<PointPair> changedPlayersToDraw;
	std::vector<PointPair> notChangedPlayersToDraw;
	std::vector<PointPair> playerMovement;

	if(history.find(frameId - 1) == history.end())
	{
		// First frame, no help from history
		cout << "++++++++++++++++++++++++++++++++++++++++ no history" << endl;
		history.insert(std::make_pair(frameId, curFrameInput));

		for (RecognizedPlayer& player : curFrameInput) {
			// Initiate last used count
			lastUpdatedPlayer.insert(std::make_pair(player.getCamerasPlayerId(), 1));

			// Debug: add players for debug-image
			changedPlayersToDraw.push_back(PointPair(player.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));
		}
	} else {
		// History is available
		// Try to find the previously known players
		cout << "++++++++++++++++++++++++++++++++++++++++ history available" << endl;

		std::vector<int> usedHistoryPlayers;
		// First loop: Try to recognize (in the input) the players we have seen in the last two frames
		// That means we give the players we are currently tracking well priority over the ones we lost.
		for (RecognizedPlayer& histPlayer : history.find(frameId - 1)->second) {

			std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId());
			if (it != lastUpdatedPlayer.end() && it->second <= 2) {
				// Handle player now and remember to skip him in the second loop
				usedHistoryPlayers.push_back(histPlayer.getCamerasPlayerId());
			} else {
				continue;
			}

			createHistory(curFrameInput, newHistoryInput, histPlayer, notChangedPlayersToDraw, changedPlayersToDraw, playerMovement);
		}

		// Second loop: Try to match the history-players we did not see for some time
		for (RecognizedPlayer& histPlayer : history.find(frameId - 1)->second) {
			std::vector<int>::iterator it = find (usedHistoryPlayers.begin(), usedHistoryPlayers.end(), histPlayer.getCamerasPlayerId());
			if (it != usedHistoryPlayers.end())
				// Already handled this player in the first loop
				continue;

			createHistory(curFrameInput, newHistoryInput, histPlayer, notChangedPlayersToDraw, changedPlayersToDraw, playerMovement);
		}

		// Handle players not used until now
		cout << "curFrameInput.size() = " << curFrameInput.size() << endl;
		// TODO Try to find a lostPlayer in the unused input

		// Add everything to history (memory)
		history.insert(std::make_pair(frameId, newHistoryInput));
	}
	
	// Debug: Create and show tracking result
	ModelImageGenerator::createFieldModel("Tracking", notChangedPlayersToDraw, playerMovement, changedPlayersToDraw);

	// Debug: print history
	printHistory();
}

void TrackingModule::createHistory(std::vector<RecognizedPlayer> &curFrameInput, std::vector<RecognizedPlayer> &newHistoryInput, RecognizedPlayer histPlayer, std::vector<PointPair> &notChangedPlayersToDraw, std::vector<PointPair> &changedPlayersToDraw, std::vector<PointPair> &playerMovement) {
	
	bool hasMatched = false;
	RecognizedPlayer nearestPlayer;
	int nearestPlayerDistance = 9999;

	cout << "History-Player #" << histPlayer.getCamerasPlayerId() << endl;
	auto curFramePlayer = std::begin(curFrameInput);
	// Loop over new input
	while (curFramePlayer != std::end(curFrameInput)) {
		cout << "+++++++++++++++++ Trying to match #" << (*curFramePlayer).getCamerasPlayerId() << endl;
		int multiplicator = 1;
		std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId());
		if (it != lastUpdatedPlayer.end())
			multiplicator = it->second;
		if (isPossiblySamePlayer(histPlayer, *curFramePlayer, 300 + multiplicator * 70)) {
			// Flag to be used later
			hasMatched = true;
			
			//cout << "0000000000000000000000000000 MATCHED ";
			
			int distance = getDistance(histPlayer, *curFramePlayer);
			if (distance < nearestPlayerDistance) {
				nearestPlayerDistance = distance;
				nearestPlayer = *curFramePlayer;
				//cout << "AND NEW BEST" << endl;
			} else {
				//cout << "but not new Best" << endl;
			}

			++curFramePlayer;
		}else {
			// Not the same player we searched in history
			++curFramePlayer;
		}
	}
	if (! hasMatched) {
		cout << "+++++++++++++++++ did not find #" << histPlayer.getCamerasPlayerId() << endl;

		// Reset last used counter
		std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId()); 
		if (it != lastUpdatedPlayer.end()) {
			if (it->second < 6) {
				it->second = it->second + 1;
				// Histplayer was not found in new input. We fill the gap temporarly
				newHistoryInput.push_back(histPlayer);
			} else {
				
				newHistoryInput.push_back(histPlayer);

				// We have not seen this player for a long time, so we forget him.
				/*auto curPlayer = std::begin(lastUpdatedPlayer);

				while (curPlayer != std::end(lastUpdatedPlayer)) {
					cout << "Forget about player #" << histPlayer.getCamerasPlayerId() << endl;
					cout << curPlayer->first << " - " << curPlayer->second << endl;
					if (histPlayer.getCamerasPlayerId() == curPlayer->first) {
						// TODO in den Fällen curPlayer = erase???
						lastUpdatedPlayer.erase(curPlayer);
					}
					++curPlayer;
				}
				// Remember, which players we lost, so we can find them later
				lostPlayers.push_back(histPlayer);*/
			}
		}
			
		
		// Debug: add players for debug image
		notChangedPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, histPlayer.getPositionInModel().x, histPlayer.getPositionInModel().y));
	} else {
		// We found the player again, add him/her to history
		RecognizedPlayer player;
		player.setCamerasPlayerId(histPlayer.getCamerasPlayerId());
		player.setPositionInModel(Point(nearestPlayer.getPositionInModel().x, nearestPlayer.getPositionInModel().y), true);
		player.setIsRed(nearestPlayer.getIsRed(), true);
		newHistoryInput.push_back(player);

		// Erase player from new input (we don't want to use him twice)
		curFramePlayer = std::begin(curFrameInput);
		while (curFramePlayer != std::end(curFrameInput)) {
			if ((*curFramePlayer).getCamerasPlayerId() == nearestPlayer.getCamerasPlayerId()) {
				// TODO in den Fällen curPlayer = erase???
				curFrameInput.erase(curFramePlayer);
				break;
			} else {
				++curFramePlayer;
			}
		}

		// Reset last used counter
		std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId()); 
		if (it != lastUpdatedPlayer.end())
			it->second = 1;

		// Debug: add players for debug image
		changedPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));
		playerMovement.push_back(PointPair(-1, histPlayer.getPositionInModel().x, histPlayer.getPositionInModel().y, player.getPositionInModel().x, player.getPositionInModel().y));
	}
}

std::vector<RecognizedPlayer> TrackingModule::getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// New list to save the merged players
	std::vector<RecognizedPlayer> mergedPlayers;
	// threshold for search
	int threshold = 200;

	// Debug: players to draw to debug-image
	std::vector<PointPair> changedPlayersToDraw;

	RecognizedPlayer rp2;
	// Input: Confidence: Wenn alle Kameras sich einig sind: hoch, sonst tief. Anfang tief, Vergangenheit nicht viel einbeziehen
	cout << "Start Loop InputMic" << endl;
	// Loop over players seen by cameraMic. Try to match them with the players from other cameras
	for (RecognizedPlayer& rp : inputMic) {
		cout << "inputMic #" << rp.getCamerasPlayerId() << endl;
		RecognizedPlayer resultingPlayerA;
		bool matchingPlayerFromCameraHud = false;
		RecognizedPlayer resultingPlayerB;
		bool matchingPlayerFromCameraMar = false;

		auto i = std::begin(inputHud);
		// Try to match player of cameraMic (rp) with players seen by cameraHud
		while (i != std::end(inputHud)) {
			cout << "inputMic-------- hud #" << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				cout << "inputMic-------- " << "!!!!!!! same" << endl;	
				resultingPlayerA = *i;
				matchingPlayerFromCameraHud = true;
				// Remove player from list, so he is not used twice
				i = inputHud.erase(i);
			}else {
				++i;
			}
		}

		i = std::begin(inputMar);
		// Try to match player of cameraMic (rp) with players seen by cameraMar
		while (i != std::end(inputMar)) {
			cout << "inputMic-------- mar #" << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				cout << "inputMic-------- " << "!!!!!!! same" << endl;	
				resultingPlayerB = *i;
				matchingPlayerFromCameraMar = true;
				// Remove player from list, so he is not used twice
				i = inputMar.erase(i);
			}else {
				//cout << "b-------- " << "different" << endl;
				++i;
			}
		}

		// See, if we matched something for this player of cameraMic (rp)
		// If yes, we merge the positions, use the color and add it to the list
		if (matchingPlayerFromCameraHud && !matchingPlayerFromCameraMar) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerA.getPositionInModel().x) / 2;
			int y = (rp.getPositionInModel().y+resultingPlayerA.getPositionInModel().y) / 2;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);
			// Debug
			changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));
		}
		if (!matchingPlayerFromCameraHud && matchingPlayerFromCameraMar) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerB.getPositionInModel().x) / 2;
			int y = (rp.getPositionInModel().y+resultingPlayerB.getPositionInModel().y) / 2;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);
			// Debug
			changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));
			
		}
		if (matchingPlayerFromCameraHud && matchingPlayerFromCameraMar) {
			RecognizedPlayer curPlayer;
			curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId());
			curPlayer.setIsRed(rp.getIsRed(), true);
			int x = (rp.getPositionInModel().x+resultingPlayerA.getPositionInModel().x+resultingPlayerB.getPositionInModel().x) / 3;
			int y = (rp.getPositionInModel().y+resultingPlayerA.getPositionInModel().y+resultingPlayerB.getPositionInModel().y) / 3;
			curPlayer.setPositionInModel(Point(x, y), true);
			mergedPlayers.push_back(curPlayer);
			// Debug
			changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId(), -1, -1, x, y));		
		}
	}

	cout << "Start Loop InputHud" << endl;
	// Loop over players seen by cameraHud. Try to match them with the players from cameraMar
	for (RecognizedPlayer& rp : inputHud) {
		cout << "inputHud #" << rp.getCamerasPlayerId() << endl;

		auto i = std::begin(inputMar);
		while (i != std::end(inputMar)) {
			cout << "inputHud-------- mar #" << (*i).getCamerasPlayerId() << endl;
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				cout << "inputHud-------- " << "!!!!!!! same" << endl;	
				RecognizedPlayer curPlayer;
				curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 300);
				curPlayer.setIsRed(rp.getIsRed(), true);
				int x = (rp.getPositionInModel().x+(*i).getPositionInModel().x) / 2;
				int y = (rp.getPositionInModel().y+(*i).getPositionInModel().y) / 2;
				curPlayer.setPositionInModel(Point(x, y), true);
				mergedPlayers.push_back(curPlayer);
				changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 300, -1, -1, x, y));
				// Remove player from list, so he is not used twice
				i = inputMar.erase(i);
			}else {
				//cout << "c-------- " << "different" << endl;
				++i;
			}
		}

	}
	cout << "inputHud.size() " << inputHud.size() << endl;

	// Debug: create image with all the players on the field
	std::vector<PointPair> referencePoints;
	std::vector<PointPair> linesToDraw;
	ModelImageGenerator::createFieldModel("Tracking-Input", referencePoints, linesToDraw, changedPlayersToDraw);
	
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
	std::map<int, int>::iterator it2 = lastUpdatedPlayer.begin();
	while(it2 != lastUpdatedPlayer.end())
	{
		std::cout<<it2->first<<" :: "<<it2->second << std::endl;
		it2++;
	}
}

bool TrackingModule::isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold)
{
	int distance = getDistance(a, b);
	cout << "--------- " << "distance: " << distance << endl;
	if (distance < threshold) {
		if (a.getIsRed() == b.getIsRed()) {
			return true;
		}
	}
	return false;
}

int TrackingModule::getDistance(RecognizedPlayer a, RecognizedPlayer b)
{
	int diffX = a.getPositionInModel().x - b.getPositionInModel().x;
	int diffY = a.getPositionInModel().y - b.getPositionInModel().y;
	if (diffX < 0)
		diffX = diffX * -1;
	if (diffY < 0)
		diffY = diffY * -1;
	return diffX + diffY;
}
