#include <TrackingModule.h>

// TODO Verbesserungsideen
//	- Frequenz von 2 fps auf 10 fps geändert: Wird besser, mann muss aber noch die Parameter etwas justieren.
//	- 10 Spielerpunkte initial setzen vs Neuerfassen/Löschen von Spielern ausprobieren
//	- Nummernerkennung schlecht. Optionen? Unzureichende Optionen: Siehe Week 0 im Excel, SIFT evtl. noch weiter verbessern?
//		o Erkennung sehr gut, wenn man weiss, dass eine Nummer sichtbar ist (siehe Testtool Sift). Grosse Fehlerrate, wenn keine Nummer sichtbar
//		o Wenn Nummernerkennung besser: Logik einbauen zum ändern der History
//	- Ideale Zuteilung von history-Spielern und input herausfinden (globales minimieren von distanz, schwarz/rot trennen)


//	- Neue Aufnahme: Framerate, mehr Kameras
//	- Merging umbenennen und weniger radikal machen



//	- Evtl. weniger mergen/ mehr Input zulassen.
//		o für kameras Gebiete der absoluten Macht definieren. Wenn camera1 meint, in dem bereich steht ein Spieler, dann steht da einer









// Erkenntnisse
//	- FPS sehr wichtig, das System der Europameisterschaft hatte 25fps
//	- Bilddaten sind nicht so gut wie gedacht: Ecke unten links schlecht abgedeckt, CamHud verwendet zu viel des Bildes für Wände, CamMar zu wenig hoch platziert.
//	- Mehr Kameras mit kleinem Feldanteil würde wohl helfen.
//	- Komplexität wird sehr schnell sehr gross. Zum Vergleich: mit den GPS-Basierten Systemen ist Segmentation, Klassifizierung und Tracking hinfällig, man weiss, wer die Daten sendet und die Position ist gratis und sehr genau.
//	- Um ein visuelles Trackingsystem aufzubauen braucht man meiner Meinung nach mehr Ressourcen

void TrackingModule::handleInput(int frameId, std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	std::vector<RecognizedPlayer> curFrameInput;
	if (frameId == 1)
		curFrameInput = getMergedInput(inputHud, inputMar, inputMic);
	else
		curFrameInput = getFullInput(inputHud, inputMar, inputMic);

	// New input to be added to the "memory"
	std::vector<RecognizedPlayer> newHistoryInput;
	// Debug: Points to print in debug-image
	std::vector<PointPair> changedPlayersToDraw;
	std::vector<PointPair> notChangedPlayersToDraw;
	std::vector<PointPair> playerMovement;

	if(history.find(frameId - 1) == history.end())
	{
		// First frame, no help from history
		Logger::log("++++++++++++++++++++++++++++++++++++++++ no history", 0);
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
		Logger::log("++++++++++++++++++++++++++++++++++++++++ history available", 0);

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
		Logger::log("curFrameInput.size() = " + std::to_string(curFrameInput.size()), 0);
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

	Logger::log("History-Player #" + std::to_string(histPlayer.getCamerasPlayerId()), 0);
	auto curFramePlayer = std::begin(curFrameInput);
	// Loop over new input
	while (curFramePlayer != std::end(curFrameInput)) {
		Logger::log("+++++++++++++++++ Trying to match #" + std::to_string((*curFramePlayer).getCamerasPlayerId()), 0);
		int multiplicator = 1;
		std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId());
		if (it != lastUpdatedPlayer.end())
			multiplicator = it->second;
		if (isPossiblySamePlayer(histPlayer, *curFramePlayer, 200 + multiplicator * 70)) {
			// Flag to be used later
			hasMatched = true;
			
			//Logger::log("0000000000000000000000000000 MATCHED ";
			
			int distance = getDistance(histPlayer, *curFramePlayer);
			if (distance < nearestPlayerDistance) {
				nearestPlayerDistance = distance;
				nearestPlayer = *curFramePlayer;
				//Logger::log( << "AND NEW BEST", 0);
			} else {
				//Logger::log( << "but not new Best", 0);
			}

			++curFramePlayer;
		}else {
			// Not the same player we searched in history
			++curFramePlayer;
		}
	}
	if (! hasMatched) {
		Logger::log("+++++++++++++++++ did not find #" + std::to_string(histPlayer.getCamerasPlayerId()), 0);

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
					Logger::log( << "Forget about player #" << histPlayer.getCamerasPlayerId(), 0);
					Logger::log( << curPlayer->first << " - " << curPlayer->second, 0);
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
	Logger::log("Start Loop InputMic", 0);
	// Loop over players seen by cameraMic. Try to match them with the players from other cameras
	for (RecognizedPlayer& rp : inputMic) {
		Logger::log("inputMic #" + std::to_string(rp.getCamerasPlayerId()), 0);
		RecognizedPlayer resultingPlayerA;
		bool matchingPlayerFromCameraHud = false;
		RecognizedPlayer resultingPlayerB;
		bool matchingPlayerFromCameraMar = false;

		auto i = std::begin(inputHud);
		// Try to match player of cameraMic (rp) with players seen by cameraHud
		while (i != std::end(inputHud)) {
			Logger::log("inputMic-------- hud #" + std::to_string((*i).getCamerasPlayerId()), 0);
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				Logger::log("inputMic-------- !!!!!!! same", 0);	
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
			Logger::log("inputMic-------- mar #" + std::to_string((*i).getCamerasPlayerId()), 0);
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				Logger::log( "inputMic-------- !!!!!!! same", 0);	
				resultingPlayerB = *i;
				matchingPlayerFromCameraMar = true;
				// Remove player from list, so he is not used twice
				i = inputMar.erase(i);
			}else {
				//Logger::log( << "b-------- " << "different", 0);
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

	Logger::log("Start Loop InputHud", 0);
	// Loop over players seen by cameraHud. Try to match them with the players from cameraMar
	for (RecognizedPlayer& rp : inputHud) {
		Logger::log("inputHud #" + std::to_string(rp.getCamerasPlayerId()), 0);

		auto i = std::begin(inputMar);
		while (i != std::end(inputMar)) {
			Logger::log("inputHud-------- mar #" + std::to_string((*i).getCamerasPlayerId()), 0);
			if (isPossiblySamePlayer(rp, *i, threshold)) {
				// Likely the same player, be sure to save the result
				Logger::log("inputHud-------- !!!!!!! same", 0);	
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
				//Logger::log("c-------- " << "different", 0);
				++i;
			}
		}

	}
	Logger::log("inputHud.size() " + std::to_string(inputHud.size()), 0);

	// Debug: create image with all the players on the field
	std::vector<PointPair> referencePoints;
	std::vector<PointPair> linesToDraw;
	ModelImageGenerator::createFieldModel("Tracking-Input", referencePoints, linesToDraw, changedPlayersToDraw);
	
	return mergedPlayers;
}

std::vector<RecognizedPlayer> TrackingModule::getFullInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// New list to save the merged players
	std::vector<RecognizedPlayer> inputPlayers;

	// Debug: players to draw to debug-image
	std::vector<PointPair> changedPlayersToDraw;

	for (RecognizedPlayer& rp : inputMic) {
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 1000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);
		// Debug
		changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 1000, -1, -1, x, y));
	}

	for (RecognizedPlayer& rp : inputHud) {
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 2000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);
		// Debug
		changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 2000, -1, -1, x, y));
	}

	/*for (RecognizedPlayer& rp : inputMar) {
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 3000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);
		// Debug
		changedPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 3000, -1, -1, x, y));
	}*/

	// Debug: create image with all the players on the field
	std::vector<PointPair> referencePoints;
	std::vector<PointPair> linesToDraw;
	ModelImageGenerator::createFieldModel("Tracking-Input", referencePoints, linesToDraw, changedPlayersToDraw);
	
	return inputPlayers;
}

void TrackingModule::printHistory()
{
	std::map<int, std::vector<RecognizedPlayer>>::iterator it = history.begin();
	while(it != history.end())
	{
		Logger::log(std::to_string(it->first) + " :: size -> " + std::to_string(it->second.size()), 0);
		for(RecognizedPlayer& player : it->second) {
		//	Logger::log(player.toString(), 0);
			Logger::log(std::to_string(it->first) + ";" + std::to_string(player.getCamerasPlayerId()) + ";" + std::to_string(player.getPositionInModel().x) + ";" + std::to_string(player.getPositionInModel().y), 0);
		}
		it++;
	}
	std::map<int, int>::iterator it2 = lastUpdatedPlayer.begin();
	while(it2 != lastUpdatedPlayer.end())
	{
		Logger::log(std::to_string(it2->first) + " :: " + std::to_string(it2->second), 0);
		it2++;
	}
}

bool TrackingModule::isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold)
{
	int distance = getDistance(a, b);
	Logger::log("--------- distance: " + std::to_string(distance), 0);
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
