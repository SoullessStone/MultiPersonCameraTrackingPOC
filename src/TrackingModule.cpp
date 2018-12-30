#include <TrackingModule.h>

// Verworfen/Zurückgestellt:
//	- Nummernerkennung schlecht. Optionen? Unzureichende Optionen: Siehe Week 0 im Excel, SIFT evtl. noch weiter verbessern?
//		o Erkennung sehr gut, wenn man weiss, dass eine Nummer sichtbar ist (siehe Testtool Sift). Grosse Fehlerrate, wenn keine Nummer sichtbar
//		o Wenn Nummernerkennung besser: Logik einbauen zum ändern der History
//	- Ideale Zuteilung von history-Spielern und input herausfinden (globales minimieren von distanz, schwarz/rot trennen)
//	- Pro Kamera: Ausschluss ungenauer Bereiche

//Done:
//	- Evtl. weniger mergen/ mehr Input zulassen.
//		o für kameras Gebiete der absoluten Macht definieren. Wenn camera1 meint, in dem bereich steht ein Spieler, dann steht da einer
//	- Frequenz von 2 fps auf 10 fps geändert: Wird besser, mann muss aber noch die Parameter etwas justieren.
//	- 6 Spielerpunkte initial setzen vs Neuerfassen/Löschen von Spielern ausprobieren
//		- Die neuen Aufnahmen haben nur 6 Spieler. Diese sind auch, mit 1-3s Unterbrüchen, immer sichtbar. Löschen ist also nicht nötig.
//	- Manuelle Korrekturmöglichkeit einführen?
//	- Besprechung mit Hudritsch bei Gopro-Übergabe. 
//		o Voraussage über Position treffen und von dort ausgehen.
//		o 2/3 näheste Punkte nehmen und Mittelwert als neue Position nehmen
//		o Die 2/3 nähesten Punkte löschen?


// Erkenntnisse
//	- FPS sehr wichtig, das System der Europameisterschaft hatte 25fps
//	- Bilddaten sind nicht so gut wie gedacht: Ecke unten links schlecht abgedeckt, CamHud verwendet zu viel des Bildes für Wände, CamMar zu wenig hoch platziert.
//	- Mehr Kameras mit kleinem Feldanteil würde wohl helfen.
//	- Komplexität wird sehr schnell sehr gross. Zum Vergleich: mit den GPS-Basierten Systemen ist Segmentation, Klassifizierung und Tracking hinfällig, man weiss, wer die Daten sendet und die Position ist gratis und sehr genau.
//	- Um ein visuelles Trackingsystem aufzubauen braucht man meiner Meinung nach mehr Ressourcen


TrackingModule::TrackingModule()
{
	initBasetruth();
}

void TrackingModule::handleInput(
	int frameId,  					// The ID of the frame
	std::vector<RecognizedPlayer> inputHud, 	// All players recognized by the first camera (cameraHud)
	std::vector<RecognizedPlayer> inputMar, 	// All players recognized by the second camera (cameraMar)
	std::vector<RecognizedPlayer> inputMic		// All players recognized by the third camera (cameraMic)
)
{
	std::vector<RecognizedPlayer> curFrameInput;
	// Initially, we trust inputHud, the most central camera. In our showcase-video, we get the correct 3 players. When wanting to do it for every video, we need some kind of initialization-construct to be added. But for the tracking product created now, I decided to leave it out.
	if (frameId == 1)
		curFrameInput = inputHud; 
	else
		curFrameInput = getFullInput(inputHud, inputMar, inputMic);

	// New input to be added to the "memory"
	std::vector<RecognizedPlayer> newHistoryInput;
	// Debug: Points to print in debug-image
	std::vector<PointPair> redPlayersToDraw;
	std::vector<PointPair> blackPlayersToDraw;
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
			if (player.getIsRed() == true)
				redPlayersToDraw.push_back(PointPair(player.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));
			else
				blackPlayersToDraw.push_back(PointPair(player.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));
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

			createHistory(curFrameInput, newHistoryInput, histPlayer, notChangedPlayersToDraw, redPlayersToDraw, blackPlayersToDraw, playerMovement);
		}

		// Second loop: Try to match the history-players we did not see for some time
		for (RecognizedPlayer& histPlayer : history.find(frameId - 1)->second) {
			std::vector<int>::iterator it = find (usedHistoryPlayers.begin(), usedHistoryPlayers.end(), histPlayer.getCamerasPlayerId());
			if (it != usedHistoryPlayers.end())
				// Already handled this player in the first loop
				continue;

			createHistory(curFrameInput, newHistoryInput, histPlayer, notChangedPlayersToDraw, redPlayersToDraw, blackPlayersToDraw, playerMovement);
		}

		// Handle players not used until now
		Logger::log("curFrameInput.size() = " + std::to_string(curFrameInput.size()), 0);

		// Add everything to history (memory)
		history.insert(std::make_pair(frameId, newHistoryInput));
	}
	
	// Debug: Create and show tracking result
	cout << frameId << endl;
	if ((frameId - 1) % 10 == 0) {
		Mat fieldModel = ModelImageGenerator::createFieldModel("Tracking", redPlayersToDraw, playerMovement, blackPlayersToDraw, notChangedPlayersToDraw, basetruth.find(frameId)->second);
		//trackingResult.push_back(fieldModel);
		/*
		// Debug: Create output for demo video
		imwrite("../resources/tools/videocreator/input/" + std::to_string(frameId) + "_tracking.jpg", fieldModel);
		*/
	} else {
		Mat fieldModel = ModelImageGenerator::createFieldModel("Tracking", redPlayersToDraw, playerMovement, blackPlayersToDraw, notChangedPlayersToDraw);
		/*
		// Debug: Create output for demo video
		imwrite("../resources/tools/videocreator/input/" + std::to_string(frameId) + "_tracking.jpg", fieldModel);
		*/
		//trackingResult.push_back(fieldModel);
	}

	// Debug: print history
	printHistory();
}

void TrackingModule::createHistory(
	std::vector<RecognizedPlayer> &curFrameInput,		// All the detected Players for the current frame
	std::vector<RecognizedPlayer> &newHistoryInput,		// The result, refound players
	RecognizedPlayer histPlayer,				// HistoryPlayer to search for in curFrameInput
	std::vector<PointPair> &notChangedPlayersToDraw,	// Debug: For the output image
	std::vector<PointPair> &redPlayersToDraw,		// Debug: For the output image
	std::vector<PointPair> &blackPlayersToDraw,		// Debug: For the output image
	std::vector<PointPair> &playerMovement			// Debug: For the output image
) {
	
	bool hasMatched = false;
	RecognizedPlayer nearestPlayer;
	int nearestPlayerDistance = 9999;

	Logger::log("History-Player #" + std::to_string(histPlayer.getCamerasPlayerId()), 0);
	auto curFramePlayer = std::begin(curFrameInput);
	// Loop over new input
	while (curFramePlayer != std::end(curFrameInput)) {
		Logger::log("+++++++++++++++++ Trying to match #" + std::to_string((*curFramePlayer).getCamerasPlayerId()), 0);
		// When we did not see the player for some frames, we widen the search radius with a multiplicator
		int multiplicator = 1;
		std::map<int, int>::iterator it = lastUpdatedPlayer.find(histPlayer.getCamerasPlayerId());
		if (it != lastUpdatedPlayer.end())
			multiplicator = it->second;
		if (isPossiblySamePlayer(histPlayer, *curFramePlayer, 200 + multiplicator * 70)) {
			// Hopefully the correct player we searched for!

			// Flag to be used later
			hasMatched = true;
			
			//Logger::log("0000000000000000000000000000 MATCHED ";
			
			// We do not want the first hit, we want the best one
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
				// We could add a logic to remove the histplayer from our memory, but I chose not to, because in this POC, the number of players is fixed.
				newHistoryInput.push_back(histPlayer);
			}
		}
			
		
		// Debug: add players for debug image
		notChangedPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, histPlayer.getPositionInModel().x, histPlayer.getPositionInModel().y));
		if (histPlayer.getIsRed() == true)
			redPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, histPlayer.getPositionInModel().x, histPlayer.getPositionInModel().y));
		else
			blackPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, histPlayer.getPositionInModel().x, histPlayer.getPositionInModel().y));

	} else {
		// We found the player again, add him/her to history
		RecognizedPlayer player;
		player.setCamerasPlayerId(histPlayer.getCamerasPlayerId());
		player.setPositionInModel(Point(nearestPlayer.getPositionInModel().x, nearestPlayer.getPositionInModel().y), true);
		player.setIsRed(nearestPlayer.getIsRed(), true);
		// Also update the history (yellow line in the image "Tracking")
		std::vector<PointPair> positionHistory = histPlayer.positionHistory;
		/*
		// Debug: Set number for PP-ID, so we can paint in other color.
		int color = -1;
		if (nearestPlayer.getIsRed())
			color = -2;
		PointPair newPP(color, histPlayer.getPositionInModel().x,histPlayer.getPositionInModel().y, nearestPlayer.getPositionInModel().x, nearestPlayer.getPositionInModel().y);
		*/
		PointPair newPP(-1, histPlayer.getPositionInModel().x,histPlayer.getPositionInModel().y, nearestPlayer.getPositionInModel().x, nearestPlayer.getPositionInModel().y);
		positionHistory.insert(positionHistory.begin(), newPP);
		player.positionHistory = positionHistory;

		newHistoryInput.push_back(player);

		// Erase player from new input (we don't want to use him twice)
		curFramePlayer = std::begin(curFrameInput);
		while (curFramePlayer != std::end(curFrameInput)) {
			if ((*curFramePlayer).getCamerasPlayerId() == nearestPlayer.getCamerasPlayerId()) {
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
		if (histPlayer.getIsRed() == true)
			redPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));
		else
			blackPlayersToDraw.push_back(PointPair(histPlayer.getCamerasPlayerId(), -1, -1, player.getPositionInModel().x, player.getPositionInModel().y));

		// Debug: Show player history on final output.
		int i = 0;
		for (PointPair& pp : player.positionHistory) {
			if (i == 20)
				break;
			i++;
			playerMovement.push_back(pp);
		}
	}
}

std::vector<RecognizedPlayer> TrackingModule::getFullInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// New list to save the merged players
	std::vector<RecognizedPlayer> inputPlayers;

	// Debug: players to draw to debug-image
	std::vector<PointPair> redPlayersToDraw;
	std::vector<PointPair> blackPlayersToDraw;

	for (RecognizedPlayer& rp : inputMic) {
		// Handle cameraMic. We add 1000 to the Id to be able to distinguish between the players for debugging-reasons
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 1000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);

		// Debug: Draw players onto the field
		if (rp.getIsRed() == true)
			redPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 1000, -1, -1, x, y));
		else
			blackPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 1000, -1, -1, x, y));
	}

	for (RecognizedPlayer& rp : inputHud) {
		// Handle cameraMic. We add 2000 to the Id to be able to distinguish between the players for debugging-reasons
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 2000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);

		// Debug: Draw players onto the field
		if (rp.getIsRed() == true)
			redPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 2000, -1, -1, x, y));
		else
			blackPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 2000, -1, -1, x, y));
	}

	for (RecognizedPlayer& rp : inputMar) {
		// Handle cameraMic. We add 3000 to the Id to be able to distinguish between the players for debugging-reasons
		RecognizedPlayer curPlayer;
		curPlayer.setCamerasPlayerId(rp.getCamerasPlayerId() + 3000);
		curPlayer.setIsRed(rp.getIsRed(), true);
		int x = rp.getPositionInModel().x;
		int y = rp.getPositionInModel().y;
		curPlayer.setPositionInModel(Point(x, y), true);
		inputPlayers.push_back(curPlayer);

		// Debug: Draw players onto the field
		if (rp.getIsRed() == true)
			redPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 3000, -1, -1, x, y));
		else
			blackPlayersToDraw.push_back(PointPair(rp.getCamerasPlayerId() + 3000, -1, -1, x, y));
	}

	// Debug: create image with all the players on the field
	std::vector<PointPair> linesToDraw;
	ModelImageGenerator::createFieldModel("Tracking-Input", redPlayersToDraw, linesToDraw, blackPlayersToDraw);
	
	return inputPlayers;
}

// Method to print the History. More of a debug feature
void TrackingModule::printHistory()
{
	std::map<int, std::vector<RecognizedPlayer>>::iterator it = history.begin();
	while(it != history.end())
	{
		Logger::log(std::to_string(it->first) + " :: size -> " + std::to_string(it->second.size()), 0);
		for(RecognizedPlayer& player : it->second) {
		//	Logger::log(player.toString(), 0);
			Logger::log(std::to_string(it->first) + ";" + std::to_string(player.getCamerasPlayerId()) + ";" + std::to_string(player.getPositionInModel().x) + ";" + std::to_string(player.getPositionInModel().y), 1);
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

// Delivers all the HistoryPlayerIds
std::vector<int> TrackingModule::getHistoryPlayerIds() {
	std::vector<int> result;
	std::map<int, std::vector<RecognizedPlayer>>::iterator it = history.begin();
	while(it != history.end())
	{
		for(RecognizedPlayer& player : it->second) {
			result.push_back(player.getCamerasPlayerId());
		}
		// One Frame tells us everything
		break;
	}
	return result;
}

// Method to apply a correction to history
void TrackingModule::applyCorrection(int playerId, int frameId, Point newPosition) {
	// Find the historyPlayerList for a specific frame
	if(history.find(frameId) != history.end())
	{
		// Loop over the List
		for (RecognizedPlayer& histPlayer : history.find(frameId)->second) {
			if (histPlayer.getCamerasPlayerId() == playerId) {
				// If we found the player we want to change, we do it.
				histPlayer.setPositionInModel(newPosition, true);
				Logger::log("Applied correction: " + std::to_string(playerId) + ", " + std::to_string(frameId), 1);
			}
		}
	} else {
		Logger::log("Could not apply correction for " + std::to_string(frameId), 1);
	}
}

// As seen here: https://docs.opencv.org/3.4.0/d5/d57/videowriter_basic_8cpp-example.html
void TrackingModule::createVideo() {
	Size size = trackingResult.at(0).size();

	VideoWriter writer;
	int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');	// select desired codec (must be available at runtime)
	double fps = 10.0;					// framerate of the created video stream
	string filename = "./live.avi";				// name of the output video file
	writer.open(filename, codec, fps, size, true);
	// check if we succeeded
	if (!writer.isOpened()) {
		cerr << "Could not open the output video file for write\n";
	}

	for(int i=0; i<trackingResult.size(); i++){
		writer.write(trackingResult.at(i));
	}
	cout << "Finished writing" << endl;
}

// Compares two players and tells, if it cound be the same
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

// Return the distance between two players
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

std::vector<RecognizedPlayer> TrackingModule::getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic)
{
	// Used to be more logic here. Now we just return the players inputHud.
	return inputHud;
}

void TrackingModule::initBasetruth()
{
	// Dev Info: You generated that with the excel file in the documentation folder :)
	std::vector<PointPair> vector1;
	std::vector<PointPair> vector11;
	std::vector<PointPair> vector21;
	std::vector<PointPair> vector31;
	std::vector<PointPair> vector41;
	std::vector<PointPair> vector51;
	std::vector<PointPair> vector61;
	std::vector<PointPair> vector71;
	std::vector<PointPair> vector81;
	std::vector<PointPair> vector91;
	std::vector<PointPair> vector101;
	std::vector<PointPair> vector111;
	std::vector<PointPair> vector121;
	std::vector<PointPair> vector131;
	std::vector<PointPair> vector141;
	std::vector<PointPair> vector151;
	std::vector<PointPair> vector161;
	std::vector<PointPair> vector171;
	std::vector<PointPair> vector181;
	std::vector<PointPair> vector191;
	std::vector<PointPair> vector201;
	std::vector<PointPair> vector211;
	std::vector<PointPair> vector221;
	std::vector<PointPair> vector231;
	std::vector<PointPair> vector241;
	std::vector<PointPair> vector251;
	std::vector<PointPair> vector261;
	std::vector<PointPair> vector271;
	std::vector<PointPair> vector281;
	std::vector<PointPair> vector291;
	std::vector<PointPair> vector301;
	std::vector<PointPair> vector311;
	std::vector<PointPair> vector321;
	std::vector<PointPair> vector331;
	std::vector<PointPair> vector341;
	std::vector<PointPair> vector351;
	std::vector<PointPair> vector361;
	std::vector<PointPair> vector371;
	std::vector<PointPair> vector381;
	std::vector<PointPair> vector391;
	std::vector<PointPair> vector401;
	std::vector<PointPair> vector411;
	std::vector<PointPair> vector421;
	std::vector<PointPair> vector431;
	std::vector<PointPair> vector441;
	std::vector<PointPair> vector451;
	std::vector<PointPair> vector461;
	std::vector<PointPair> vector471;
	std::vector<PointPair> vector481;
	std::vector<PointPair> vector491;
	std::vector<PointPair> vector501;
	std::vector<PointPair> vector511;
	std::vector<PointPair> vector521;
	std::vector<PointPair> vector531;
	std::vector<PointPair> vector541;
	std::vector<PointPair> vector551;
	std::vector<PointPair> vector561;
	std::vector<PointPair> vector571;
	std::vector<PointPair> vector581;
	std::vector<PointPair> vector591;
	std::vector<PointPair> vector601;
	std::vector<PointPair> vector611;
	std::vector<PointPair> vector621;
	std::vector<PointPair> vector631;
	std::vector<PointPair> vector641;
	std::vector<PointPair> vector651;
	std::vector<PointPair> vector661;
	std::vector<PointPair> vector671;
	std::vector<PointPair> vector681;
	std::vector<PointPair> vector691;
	std::vector<PointPair> vector701;
	std::vector<PointPair> vector711;
	std::vector<PointPair> vector721;
	std::vector<PointPair> vector731;
	std::vector<PointPair> vector741;
	std::vector<PointPair> vector751;
	std::vector<PointPair> vector761;
	std::vector<PointPair> vector771;
	std::vector<PointPair> vector781;
	std::vector<PointPair> vector791;
	std::vector<PointPair> vector801;
	std::vector<PointPair> vector811;
	std::vector<PointPair> vector821;
	std::vector<PointPair> vector831;
	std::vector<PointPair> vector841;
	std::vector<PointPair> vector851;
	std::vector<PointPair> vector861;
	std::vector<PointPair> vector871;
	std::vector<PointPair> vector881;

	vector1.push_back(PointPair(1,604,282,-1,-1));
	vector1.push_back(PointPair(107,132,295,-1,-1));
	vector1.push_back(PointPair(103,269,381,-1,-1));
	vector1.push_back(PointPair(102,289,293,-1,-1));
	vector1.push_back(PointPair(4,594,528,-1,-1));
	vector1.push_back(PointPair(5,495,57,-1,-1));
	vector11.push_back(PointPair(1,564,274,-1,-1));
	vector11.push_back(PointPair(107,115,295,-1,-1));
	vector11.push_back(PointPair(103,253,382,-1,-1));
	vector11.push_back(PointPair(102,267,276,-1,-1));
	vector11.push_back(PointPair(4,513,520,-1,-1));
	vector11.push_back(PointPair(5,406,58,-1,-1));
	vector21.push_back(PointPair(1,511,278,-1,-1));
	vector21.push_back(PointPair(107,103,274,-1,-1));
	vector21.push_back(PointPair(103,238,358,-1,-1));
	vector21.push_back(PointPair(102,258,197,-1,-1));
	vector21.push_back(PointPair(4,467,523,-1,-1));
	vector21.push_back(PointPair(5,377,79,-1,-1));
	vector31.push_back(PointPair(1,430,311,-1,-1));
	vector31.push_back(PointPair(107,113,298,-1,-1));
	vector31.push_back(PointPair(103,275,351,-1,-1));
	vector31.push_back(PointPair(102,269,211,-1,-1));
	vector31.push_back(PointPair(4,449,557,-1,-1));
	vector31.push_back(PointPair(5,316,47,-1,-1));
	vector41.push_back(PointPair(1,338,281,-1,-1));
	vector41.push_back(PointPair(107,100,291,-1,-1));
	vector41.push_back(PointPair(103,283,336,-1,-1));
	vector41.push_back(PointPair(102,269,231,-1,-1));
	vector41.push_back(PointPair(4,423,554,-1,-1));
	vector41.push_back(PointPair(5,287,44,-1,-1));
	vector51.push_back(PointPair(1,337,279,-1,-1));
	vector51.push_back(PointPair(107,80,300,-1,-1));
	vector51.push_back(PointPair(103,258,351,-1,-1));
	vector51.push_back(PointPair(102,269,237,-1,-1));
	vector51.push_back(PointPair(4,411,532,-1,-1));
	vector51.push_back(PointPair(5,343,48,-1,-1));
	vector61.push_back(PointPair(1,393,328,-1,-1));
	vector61.push_back(PointPair(107,93,303,-1,-1));
	vector61.push_back(PointPair(103,371,386,-1,-1));
	vector61.push_back(PointPair(102,262,279,-1,-1));
	vector61.push_back(PointPair(4,409,511,-1,-1));
	vector61.push_back(PointPair(5,343,48,-1,-1));
	vector71.push_back(PointPair(1,414,296,-1,-1));
	vector71.push_back(PointPair(107,94,315,-1,-1));
	vector71.push_back(PointPair(103,434,501,-1,-1));
	vector71.push_back(PointPair(102,271,324,-1,-1));
	vector71.push_back(PointPair(4,373,514,-1,-1));
	vector71.push_back(PointPair(5,399,70,-1,-1));
	vector81.push_back(PointPair(1,370,273,-1,-1));
	vector81.push_back(PointPair(107,102,334,-1,-1));
	vector81.push_back(PointPair(103,334,484,-1,-1));
	vector81.push_back(PointPair(102,262,384,-1,-1));
	vector81.push_back(PointPair(4,271,494,-1,-1));
	vector81.push_back(PointPair(5,340,77,-1,-1));
	vector91.push_back(PointPair(1,361,269,-1,-1));
	vector91.push_back(PointPair(107,98,297,-1,-1));
	vector91.push_back(PointPair(103,223,378,-1,-1));
	vector91.push_back(PointPair(102,213,282,-1,-1));
	vector91.push_back(PointPair(4,165,464,-1,-1));
	vector91.push_back(PointPair(5,268,46,-1,-1));
	vector101.push_back(PointPair(1,317,272,-1,-1));
	vector101.push_back(PointPair(107,97,297,-1,-1));
	vector101.push_back(PointPair(103,190,314,-1,-1));
	vector101.push_back(PointPair(102,155,188,-1,-1));
	vector101.push_back(PointPair(4,119,423,-1,-1));
	vector101.push_back(PointPair(5,178,83,-1,-1));
	vector111.push_back(PointPair(1,317,275,-1,-1));
	vector111.push_back(PointPair(107,102,303,-1,-1));
	vector111.push_back(PointPair(103,161,298,-1,-1));
	vector111.push_back(PointPair(102,169,180,-1,-1));
	vector111.push_back(PointPair(4,114,400,-1,-1));
	vector111.push_back(PointPair(5,126,92,-1,-1));
	vector121.push_back(PointPair(1,428,283,-1,-1));
	vector121.push_back(PointPair(107,136,291,-1,-1));
	vector121.push_back(PointPair(103,262,366,-1,-1));
	vector121.push_back(PointPair(102,287,189,-1,-1));
	vector121.push_back(PointPair(4,154,401,-1,-1));
	vector121.push_back(PointPair(5,311,142,-1,-1));
	vector131.push_back(PointPair(1,540,297,-1,-1));
	vector131.push_back(PointPair(107,203,196,-1,-1));
	vector131.push_back(PointPair(103,450,396,-1,-1));
	vector131.push_back(PointPair(102,386,195,-1,-1));
	vector131.push_back(PointPair(4,289,366,-1,-1));
	vector131.push_back(PointPair(5,474,159,-1,-1));
	vector141.push_back(PointPair(1,655,298,-1,-1));
	vector141.push_back(PointPair(107,346,90,-1,-1));
	vector141.push_back(PointPair(103,635,396,-1,-1));
	vector141.push_back(PointPair(102,438,144,-1,-1));
	vector141.push_back(PointPair(4,455,324,-1,-1));
	vector141.push_back(PointPair(5,593,155,-1,-1));
	vector151.push_back(PointPair(1,743,298,-1,-1));
	vector151.push_back(PointPair(107,459,89,-1,-1));
	vector151.push_back(PointPair(103,686,388,-1,-1));
	vector151.push_back(PointPair(102,394,260,-1,-1));
	vector151.push_back(PointPair(4,537,287,-1,-1));
	vector151.push_back(PointPair(5,661,127,-1,-1));
	vector161.push_back(PointPair(1,743,298,-1,-1));
	vector161.push_back(PointPair(107,544,142,-1,-1));
	vector161.push_back(PointPair(103,704,415,-1,-1));
	vector161.push_back(PointPair(102,412,322,-1,-1));
	vector161.push_back(PointPair(4,568,285,-1,-1));
	vector161.push_back(PointPair(5,707,152,-1,-1));
	vector171.push_back(PointPair(1,764,362,-1,-1));
	vector171.push_back(PointPair(107,645,159,-1,-1));
	vector171.push_back(PointPair(103,715,505,-1,-1));
	vector171.push_back(PointPair(102,458,372,-1,-1));
	vector171.push_back(PointPair(4,556,384,-1,-1));
	vector171.push_back(PointPair(5,743,186,-1,-1));
	vector181.push_back(PointPair(1,767,373,-1,-1));
	vector181.push_back(PointPair(107,725,162,-1,-1));
	vector181.push_back(PointPair(103,744,520,-1,-1));
	vector181.push_back(PointPair(102,576,326,-1,-1));
	vector181.push_back(PointPair(4,621,417,-1,-1));
	vector181.push_back(PointPair(5,781,224,-1,-1));
	vector191.push_back(PointPair(1,784,369,-1,-1));
	vector191.push_back(PointPair(107,592,120,-1,-1));
	vector191.push_back(PointPair(103,765,485,-1,-1));
	vector191.push_back(PointPair(102,511,279,-1,-1));
	vector191.push_back(PointPair(4,674,402,-1,-1));
	vector191.push_back(PointPair(5,754,187,-1,-1));
	vector201.push_back(PointPair(1,823,366,-1,-1));
	vector201.push_back(PointPair(107,469,70,-1,-1));
	vector201.push_back(PointPair(103,777,370,-1,-1));
	vector201.push_back(PointPair(102,422,281,-1,-1));
	vector201.push_back(PointPair(4,707,373,-1,-1));
	vector201.push_back(PointPair(5,727,155,-1,-1));
	vector211.push_back(PointPair(1,848,325,-1,-1));
	vector211.push_back(PointPair(107,442,112,-1,-1));
	vector211.push_back(PointPair(103,769,273,-1,-1));
	vector211.push_back(PointPair(102,363,313,-1,-1));
	vector211.push_back(PointPair(4,741,365,-1,-1));
	vector211.push_back(PointPair(5,773,145,-1,-1));
	vector221.push_back(PointPair(1,890,334,-1,-1));
	vector221.push_back(PointPair(107,468,168,-1,-1));
	vector221.push_back(PointPair(103,763,158,-1,-1));
	vector221.push_back(PointPair(102,347,382,-1,-1));
	vector221.push_back(PointPair(4,766,372,-1,-1));
	vector221.push_back(PointPair(5,829,144,-1,-1));
	vector231.push_back(PointPair(1,836,330,-1,-1));
	vector231.push_back(PointPair(107,558,253,-1,-1));
	vector231.push_back(PointPair(103,722,139,-1,-1));
	vector231.push_back(PointPair(102,435,456,-1,-1));
	vector231.push_back(PointPair(4,786,385,-1,-1));
	vector231.push_back(PointPair(5,785,199,-1,-1));
	vector241.push_back(PointPair(1,788,322,-1,-1));
	vector241.push_back(PointPair(107,655,298,-1,-1));
	vector241.push_back(PointPair(103,721,111,-1,-1));
	vector241.push_back(PointPair(102,542,508,-1,-1));
	vector241.push_back(PointPair(4,817,408,-1,-1));
	vector241.push_back(PointPair(5,798,246,-1,-1));
	vector251.push_back(PointPair(1,798,384,-1,-1));
	vector251.push_back(PointPair(107,731,345,-1,-1));
	vector251.push_back(PointPair(103,670,142,-1,-1));
	vector251.push_back(PointPair(102,639,553,-1,-1));
	vector251.push_back(PointPair(4,841,478,-1,-1));
	vector251.push_back(PointPair(5,838,223,-1,-1));
	vector261.push_back(PointPair(1,816,432,-1,-1));
	vector261.push_back(PointPair(107,859,394,-1,-1));
	vector261.push_back(PointPair(103,649,162,-1,-1));
	vector261.push_back(PointPair(102,703,550,-1,-1));
	vector261.push_back(PointPair(4,789,520,-1,-1));
	vector261.push_back(PointPair(5,810,245,-1,-1));
	vector271.push_back(PointPair(1,852,501,-1,-1));
	vector271.push_back(PointPair(107,906,429,-1,-1));
	vector271.push_back(PointPair(103,607,291,-1,-1));
	vector271.push_back(PointPair(102,716,550,-1,-1));
	vector271.push_back(PointPair(4,762,582,-1,-1));
	vector271.push_back(PointPair(5,767,220,-1,-1));
	vector281.push_back(PointPair(1,879,582,-1,-1));
	vector281.push_back(PointPair(107,836,443,-1,-1));
	vector281.push_back(PointPair(103,619,350,-1,-1));
	vector281.push_back(PointPair(102,793,546,-1,-1));
	vector281.push_back(PointPair(4,819,572,-1,-1));
	vector281.push_back(PointPair(5,754,179,-1,-1));
	vector291.push_back(PointPair(1,974,570,-1,-1));
	vector291.push_back(PointPair(107,756,429,-1,-1));
	vector291.push_back(PointPair(103,656,355,-1,-1));
	vector291.push_back(PointPair(102,812,522,-1,-1));
	vector291.push_back(PointPair(4,859,563,-1,-1));
	vector291.push_back(PointPair(5,873,120,-1,-1));
	vector301.push_back(PointPair(1,977,472,-1,-1));
	vector301.push_back(PointPair(107,722,394,-1,-1));
	vector301.push_back(PointPair(103,657,297,-1,-1));
	vector301.push_back(PointPair(102,787,504,-1,-1));
	vector301.push_back(PointPair(4,867,567,-1,-1));
	vector301.push_back(PointPair(5,888,56,-1,-1));
	vector311.push_back(PointPair(1,925,416,-1,-1));
	vector311.push_back(PointPair(107,635,350,-1,-1));
	vector311.push_back(PointPair(103,662,204,-1,-1));
	vector311.push_back(PointPair(102,734,496,-1,-1));
	vector311.push_back(PointPair(4,814,535,-1,-1));
	vector311.push_back(PointPair(5,887,56,-1,-1));
	vector321.push_back(PointPair(1,891,388,-1,-1));
	vector321.push_back(PointPair(107,518,301,-1,-1));
	vector321.push_back(PointPair(103,655,127,-1,-1));
	vector321.push_back(PointPair(102,688,446,-1,-1));
	vector321.push_back(PointPair(4,796,521,-1,-1));
	vector321.push_back(PointPair(5,887,56,-1,-1));
	vector331.push_back(PointPair(1,913,355,-1,-1));
	vector331.push_back(PointPair(107,454,296,-1,-1));
	vector331.push_back(PointPair(103,592,124,-1,-1));
	vector331.push_back(PointPair(102,688,446,-1,-1));
	vector331.push_back(PointPair(4,733,553,-1,-1));
	vector331.push_back(PointPair(5,789,55,-1,-1));
	vector341.push_back(PointPair(1,840,321,-1,-1));
	vector341.push_back(PointPair(107,409,297,-1,-1));
	vector341.push_back(PointPair(103,528,136,-1,-1));
	vector341.push_back(PointPair(102,636,467,-1,-1));
	vector341.push_back(PointPair(4,623,554,-1,-1));
	vector341.push_back(PointPair(5,712,94,-1,-1));
	vector351.push_back(PointPair(1,653,347,-1,-1));
	vector351.push_back(PointPair(107,321,296,-1,-1));
	vector351.push_back(PointPair(103,444,153,-1,-1));
	vector351.push_back(PointPair(102,537,439,-1,-1));
	vector351.push_back(PointPair(4,572,567,-1,-1));
	vector351.push_back(PointPair(5,635,98,-1,-1));
	vector361.push_back(PointPair(1,538,296,-1,-1));
	vector361.push_back(PointPair(107,277,297,-1,-1));
	vector361.push_back(PointPair(103,347,157,-1,-1));
	vector361.push_back(PointPair(102,475,364,-1,-1));
	vector361.push_back(PointPair(4,480,563,-1,-1));
	vector361.push_back(PointPair(5,642,105,-1,-1));
	vector371.push_back(PointPair(1,425,236,-1,-1));
	vector371.push_back(PointPair(107,256,297,-1,-1));
	vector371.push_back(PointPair(103,340,152,-1,-1));
	vector371.push_back(PointPair(102,441,368,-1,-1));
	vector371.push_back(PointPair(4,489,543,-1,-1));
	vector371.push_back(PointPair(5,600,140,-1,-1));
	vector381.push_back(PointPair(1,318,314,-1,-1));
	vector381.push_back(PointPair(107,202,304,-1,-1));
	vector381.push_back(PointPair(103,411,198,-1,-1));
	vector381.push_back(PointPair(102,390,436,-1,-1));
	vector381.push_back(PointPair(4,493,518,-1,-1));
	vector381.push_back(PointPair(5,615,225,-1,-1));
	vector391.push_back(PointPair(1,253,464,-1,-1));
	vector391.push_back(PointPair(107,142,314,-1,-1));
	vector391.push_back(PointPair(103,558,267,-1,-1));
	vector391.push_back(PointPair(102,396,462,-1,-1));
	vector391.push_back(PointPair(4,480,545,-1,-1));
	vector391.push_back(PointPair(5,613,294,-1,-1));
	vector401.push_back(PointPair(1,261,526,-1,-1));
	vector401.push_back(PointPair(107,123,315,-1,-1));
	vector401.push_back(PointPair(103,507,310,-1,-1));
	vector401.push_back(PointPair(102,450,482,-1,-1));
	vector401.push_back(PointPair(4,474,548,-1,-1));
	vector401.push_back(PointPair(5,509,343,-1,-1));
	vector411.push_back(PointPair(1,261,507,-1,-1));
	vector411.push_back(PointPair(107,96,299,-1,-1));
	vector411.push_back(PointPair(103,382,218,-1,-1));
	vector411.push_back(PointPair(102,342,449,-1,-1));
	vector411.push_back(PointPair(4,433,510,-1,-1));
	vector411.push_back(PointPair(5,361,241,-1,-1));
	vector421.push_back(PointPair(1,235,451,-1,-1));
	vector421.push_back(PointPair(107,91,273,-1,-1));
	vector421.push_back(PointPair(103,317,66,-1,-1));
	vector421.push_back(PointPair(102,279,364,-1,-1));
	vector421.push_back(PointPair(4,441,431,-1,-1));
	vector421.push_back(PointPair(5,318,68,-1,-1));
	vector431.push_back(PointPair(1,225,414,-1,-1));
	vector431.push_back(PointPair(107,91,273,-1,-1));
	vector431.push_back(PointPair(103,292,17,-1,-1));
	vector431.push_back(PointPair(102,260,278,-1,-1));
	vector431.push_back(PointPair(4,474,388,-1,-1));
	vector431.push_back(PointPair(5,325,56,-1,-1));
	vector441.push_back(PointPair(1,253,352,-1,-1));
	vector441.push_back(PointPair(107,91,246,-1,-1));
	vector441.push_back(PointPair(103,210,18,-1,-1));
	vector441.push_back(PointPair(102,250,270,-1,-1));
	vector441.push_back(PointPair(4,501,322,-1,-1));
	vector441.push_back(PointPair(5,341,48,-1,-1));
	vector451.push_back(PointPair(1,288,314,-1,-1));
	vector451.push_back(PointPair(107,68,237,-1,-1));
	vector451.push_back(PointPair(103,169,61,-1,-1));
	vector451.push_back(PointPair(102,216,309,-1,-1));
	vector451.push_back(PointPair(4,521,314,-1,-1));
	vector451.push_back(PointPair(5,336,104,-1,-1));
	vector461.push_back(PointPair(1,375,297,-1,-1));
	vector461.push_back(PointPair(107,90,249,-1,-1));
	vector461.push_back(PointPair(103,230,165,-1,-1));
	vector461.push_back(PointPair(102,193,396,-1,-1));
	vector461.push_back(PointPair(4,559,339,-1,-1));
	vector461.push_back(PointPair(5,391,175,-1,-1));
	vector471.push_back(PointPair(1,421,325,-1,-1));
	vector471.push_back(PointPair(107,107,281,-1,-1));
	vector471.push_back(PointPair(103,267,225,-1,-1));
	vector471.push_back(PointPair(102,220,512,-1,-1));
	vector471.push_back(PointPair(4,595,381,-1,-1));
	vector471.push_back(PointPair(5,455,206,-1,-1));
	vector481.push_back(PointPair(1,443,407,-1,-1));
	vector481.push_back(PointPair(107,130,290,-1,-1));
	vector481.push_back(PointPair(103,299,259,-1,-1));
	vector481.push_back(PointPair(102,258,521,-1,-1));
	vector481.push_back(PointPair(4,622,431,-1,-1));
	vector481.push_back(PointPair(5,438,249,-1,-1));
	vector491.push_back(PointPair(1,489,437,-1,-1));
	vector491.push_back(PointPair(107,195,287,-1,-1));
	vector491.push_back(PointPair(103,374,296,-1,-1));
	vector491.push_back(PointPair(102,267,526,-1,-1));
	vector491.push_back(PointPair(4,657,446,-1,-1));
	vector491.push_back(PointPair(5,309,345,-1,-1));
	vector501.push_back(PointPair(1,503,445,-1,-1));
	vector501.push_back(PointPair(107,202,368,-1,-1));
	vector501.push_back(PointPair(103,360,263,-1,-1));
	vector501.push_back(PointPair(102,203,483,-1,-1));
	vector501.push_back(PointPair(4,663,416,-1,-1));
	vector501.push_back(PointPair(5,226,428,-1,-1));
	vector511.push_back(PointPair(1,471,438,-1,-1));
	vector511.push_back(PointPair(107,142,351,-1,-1));
	vector511.push_back(PointPair(103,180,197,-1,-1));
	vector511.push_back(PointPair(102,103,448,-1,-1));
	vector511.push_back(PointPair(4,667,348,-1,-1));
	vector511.push_back(PointPair(5,94,419,-1,-1));
	vector521.push_back(PointPair(1,476,422,-1,-1));
	vector521.push_back(PointPair(107,110,325,-1,-1));
	vector521.push_back(PointPair(103,87,206,-1,-1));
	vector521.push_back(PointPair(102,64,443,-1,-1));
	vector521.push_back(PointPair(4,692,327,-1,-1));
	vector521.push_back(PointPair(5,112,495,-1,-1));
	vector531.push_back(PointPair(1,510,395,-1,-1));
	vector531.push_back(PointPair(107,94,297,-1,-1));
	vector531.push_back(PointPair(103,72,194,-1,-1));
	vector531.push_back(PointPair(102,61,422,-1,-1));
	vector531.push_back(PointPair(4,728,303,-1,-1));
	vector531.push_back(PointPair(5,165,518,-1,-1));
	vector541.push_back(PointPair(1,549,364,-1,-1));
	vector541.push_back(PointPair(107,82,297,-1,-1));
	vector541.push_back(PointPair(103,62,170,-1,-1));
	vector541.push_back(PointPair(102,60,409,-1,-1));
	vector541.push_back(PointPair(4,764,293,-1,-1));
	vector541.push_back(PointPair(5,255,517,-1,-1));
	vector551.push_back(PointPair(1,625,329,-1,-1));
	vector551.push_back(PointPair(107,72,297,-1,-1));
	vector551.push_back(PointPair(103,61,126,-1,-1));
	vector551.push_back(PointPair(102,87,384,-1,-1));
	vector551.push_back(PointPair(4,785,295,-1,-1));
	vector551.push_back(PointPair(5,323,517,-1,-1));
	vector561.push_back(PointPair(1,659,304,-1,-1));
	vector561.push_back(PointPair(107,72,297,-1,-1));
	vector561.push_back(PointPair(103,110,113,-1,-1));
	vector561.push_back(PointPair(102,119,410,-1,-1));
	vector561.push_back(PointPair(4,807,295,-1,-1));
	vector561.push_back(PointPair(5,402,501,-1,-1));
	vector571.push_back(PointPair(1,690,273,-1,-1));
	vector571.push_back(PointPair(107,72,297,-1,-1));
	vector571.push_back(PointPair(103,141,106,-1,-1));
	vector571.push_back(PointPair(102,129,421,-1,-1));
	vector571.push_back(PointPair(4,802,259,-1,-1));
	vector571.push_back(PointPair(5,468,461,-1,-1));
	vector581.push_back(PointPair(1,716,244,-1,-1));
	vector581.push_back(PointPair(107,72,297,-1,-1));
	vector581.push_back(PointPair(103,150,102,-1,-1));
	vector581.push_back(PointPair(102,121,409,-1,-1));
	vector581.push_back(PointPair(4,787,246,-1,-1));
	vector581.push_back(PointPair(5,513,443,-1,-1));
	vector591.push_back(PointPair(1,722,234,-1,-1));
	vector591.push_back(PointPair(107,83,299,-1,-1));
	vector591.push_back(PointPair(103,147,103,-1,-1));
	vector591.push_back(PointPair(102,143,437,-1,-1));
	vector591.push_back(PointPair(4,763,249,-1,-1));
	vector591.push_back(PointPair(5,549,440,-1,-1));
	vector601.push_back(PointPair(1,722,190,-1,-1));
	vector601.push_back(PointPair(107,96,291,-1,-1));
	vector601.push_back(PointPair(103,204,104,-1,-1));
	vector601.push_back(PointPair(102,180,468,-1,-1));
	vector601.push_back(PointPair(4,749,253,-1,-1));
	vector601.push_back(PointPair(5,592,417,-1,-1));
	vector611.push_back(PointPair(1,708,175,-1,-1));
	vector611.push_back(PointPair(107,196,283,-1,-1));
	vector611.push_back(PointPair(103,287,99,-1,-1));
	vector611.push_back(PointPair(102,243,480,-1,-1));
	vector611.push_back(PointPair(4,735,256,-1,-1));
	vector611.push_back(PointPair(5,609,410,-1,-1));
	vector621.push_back(PointPair(1,660,153,-1,-1));
	vector621.push_back(PointPair(107,251,289,-1,-1));
	vector621.push_back(PointPair(103,373,111,-1,-1));
	vector621.push_back(PointPair(102,312,484,-1,-1));
	vector621.push_back(PointPair(4,702,251,-1,-1));
	vector621.push_back(PointPair(5,609,410,-1,-1));
	vector631.push_back(PointPair(1,628,174,-1,-1));
	vector631.push_back(PointPair(107,345,304,-1,-1));
	vector631.push_back(PointPair(103,436,127,-1,-1));
	vector631.push_back(PointPair(102,307,511,-1,-1));
	vector631.push_back(PointPair(4,708,219,-1,-1));
	vector631.push_back(PointPair(5,609,410,-1,-1));
	vector641.push_back(PointPair(1,603,189,-1,-1));
	vector641.push_back(PointPair(107,378,311,-1,-1));
	vector641.push_back(PointPair(103,466,104,-1,-1));
	vector641.push_back(PointPair(102,383,527,-1,-1));
	vector641.push_back(PointPair(4,643,243,-1,-1));
	vector641.push_back(PointPair(5,609,410,-1,-1));
	vector651.push_back(PointPair(1,613,179,-1,-1));
	vector651.push_back(PointPair(107,435,327,-1,-1));
	vector651.push_back(PointPair(103,468,140,-1,-1));
	vector651.push_back(PointPair(102,449,539,-1,-1));
	vector651.push_back(PointPair(4,640,272,-1,-1));
	vector651.push_back(PointPair(5,609,410,-1,-1));
	vector661.push_back(PointPair(1,640,175,-1,-1));
	vector661.push_back(PointPair(107,435,330,-1,-1));
	vector661.push_back(PointPair(103,468,170,-1,-1));
	vector661.push_back(PointPair(102,508,530,-1,-1));
	vector661.push_back(PointPair(4,610,300,-1,-1));
	vector661.push_back(PointPair(5,642,429,-1,-1));
	vector671.push_back(PointPair(1,652,182,-1,-1));
	vector671.push_back(PointPair(107,453,380,-1,-1));
	vector671.push_back(PointPair(103,472,181,-1,-1));
	vector671.push_back(PointPair(102,566,519,-1,-1));
	vector671.push_back(PointPair(4,584,318,-1,-1));
	vector671.push_back(PointPair(5,650,440,-1,-1));
	vector681.push_back(PointPair(1,656,184,-1,-1));
	vector681.push_back(PointPair(107,485,436,-1,-1));
	vector681.push_back(PointPair(103,445,195,-1,-1));
	vector681.push_back(PointPair(102,626,519,-1,-1));
	vector681.push_back(PointPair(4,544,405,-1,-1));
	vector681.push_back(PointPair(5,655,465,-1,-1));
	vector691.push_back(PointPair(1,658,186,-1,-1));
	vector691.push_back(PointPair(107,497,520,-1,-1));
	vector691.push_back(PointPair(103,370,253,-1,-1));
	vector691.push_back(PointPair(102,658,535,-1,-1));
	vector691.push_back(PointPair(4,589,521,-1,-1));
	vector691.push_back(PointPair(5,681,491,-1,-1));
	vector701.push_back(PointPair(1,681,195,-1,-1));
	vector701.push_back(PointPair(107,578,562,-1,-1));
	vector701.push_back(PointPair(103,460,301,-1,-1));
	vector701.push_back(PointPair(102,716,575,-1,-1));
	vector701.push_back(PointPair(4,667,564,-1,-1));
	vector701.push_back(PointPair(5,774,532,-1,-1));
	vector711.push_back(PointPair(1,707,261,-1,-1));
	vector711.push_back(PointPair(107,577,559,-1,-1));
	vector711.push_back(PointPair(103,662,309,-1,-1));
	vector711.push_back(PointPair(102,743,567,-1,-1));
	vector711.push_back(PointPair(4,655,555,-1,-1));
	vector711.push_back(PointPair(5,781,512,-1,-1));
	vector721.push_back(PointPair(1,720,272,-1,-1));
	vector721.push_back(PointPair(107,495,451,-1,-1));
	vector721.push_back(PointPair(103,643,328,-1,-1));
	vector721.push_back(PointPair(102,717,496,-1,-1));
	vector721.push_back(PointPair(4,598,487,-1,-1));
	vector721.push_back(PointPair(5,770,477,-1,-1));
	vector731.push_back(PointPair(1,725,241,-1,-1));
	vector731.push_back(PointPair(107,447,302,-1,-1));
	vector731.push_back(PointPair(103,545,209,-1,-1));
	vector731.push_back(PointPair(102,603,479,-1,-1));
	vector731.push_back(PointPair(4,609,369,-1,-1));
	vector731.push_back(PointPair(5,769,402,-1,-1));
	vector741.push_back(PointPair(1,731,191,-1,-1));
	vector741.push_back(PointPair(107,511,170,-1,-1));
	vector741.push_back(PointPair(103,518,77,-1,-1));
	vector741.push_back(PointPair(102,486,458,-1,-1));
	vector741.push_back(PointPair(4,640,310,-1,-1));
	vector741.push_back(PointPair(5,776,351,-1,-1));
	vector751.push_back(PointPair(1,743,131,-1,-1));
	vector751.push_back(PointPair(107,649,162,-1,-1));
	vector751.push_back(PointPair(103,505,136,-1,-1));
	vector751.push_back(PointPair(102,582,460,-1,-1));
	vector751.push_back(PointPair(4,685,334,-1,-1));
	vector751.push_back(PointPair(5,830,295,-1,-1));
	vector761.push_back(PointPair(1,795,176,-1,-1));
	vector761.push_back(PointPair(107,748,191,-1,-1));
	vector761.push_back(PointPair(103,544,208,-1,-1));
	vector761.push_back(PointPair(102,766,472,-1,-1));
	vector761.push_back(PointPair(4,749,376,-1,-1));
	vector761.push_back(PointPair(5,882,259,-1,-1));
	vector771.push_back(PointPair(1,829,240,-1,-1));
	vector771.push_back(PointPair(107,804,285,-1,-1));
	vector771.push_back(PointPair(103,642,282,-1,-1));
	vector771.push_back(PointPair(102,917,484,-1,-1));
	vector771.push_back(PointPair(4,859,390,-1,-1));
	vector771.push_back(PointPair(5,925,309,-1,-1));
	vector781.push_back(PointPair(1,882,231,-1,-1));
	vector781.push_back(PointPair(107,888,339,-1,-1));
	vector781.push_back(PointPair(103,710,292,-1,-1));
	vector781.push_back(PointPair(102,1041,518,-1,-1));
	vector781.push_back(PointPair(4,941,417,-1,-1));
	vector781.push_back(PointPair(5,1052,379,-1,-1));
	vector791.push_back(PointPair(1,938,207,-1,-1));
	vector791.push_back(PointPair(107,932,442,-1,-1));
	vector791.push_back(PointPair(103,678,207,-1,-1));
	vector791.push_back(PointPair(102,978,511,-1,-1));
	vector791.push_back(PointPair(4,983,417,-1,-1));
	vector791.push_back(PointPair(5,1128,356,-1,-1));
	vector801.push_back(PointPair(1,979,161,-1,-1));
	vector801.push_back(PointPair(107,840,538,-1,-1));
	vector801.push_back(PointPair(103,602,178,-1,-1));
	vector801.push_back(PointPair(102,914,482,-1,-1));
	vector801.push_back(PointPair(4,1021,406,-1,-1));
	vector801.push_back(PointPair(5,1137,304,-1,-1));
	vector811.push_back(PointPair(1,1004,123,-1,-1));
	vector811.push_back(PointPair(107,703,571,-1,-1));
	vector811.push_back(PointPair(103,506,160,-1,-1));
	vector811.push_back(PointPair(102,784,465,-1,-1));
	vector811.push_back(PointPair(4,1009,380,-1,-1));
	vector811.push_back(PointPair(5,1128,288,-1,-1));
	vector821.push_back(PointPair(1,960,82,-1,-1));
	vector821.push_back(PointPair(107,560,512,-1,-1));
	vector821.push_back(PointPair(103,450,173,-1,-1));
	vector821.push_back(PointPair(102,598,457,-1,-1));
	vector821.push_back(PointPair(4,976,362,-1,-1));
	vector821.push_back(PointPair(5,1126,286,-1,-1));
	vector831.push_back(PointPair(1,934,83,-1,-1));
	vector831.push_back(PointPair(107,425,461,-1,-1));
	vector831.push_back(PointPair(103,360,192,-1,-1));
	vector831.push_back(PointPair(102,484,433,-1,-1));
	vector831.push_back(PointPair(4,930,386,-1,-1));
	vector831.push_back(PointPair(5,1097,301,-1,-1));
	vector841.push_back(PointPair(1,863,79,-1,-1));
	vector841.push_back(PointPair(107,285,403,-1,-1));
	vector841.push_back(PointPair(103,300,243,-1,-1));
	vector841.push_back(PointPair(102,383,399,-1,-1));
	vector841.push_back(PointPair(4,835,439,-1,-1));
	vector841.push_back(PointPair(5,1043,342,-1,-1));
	vector851.push_back(PointPair(1,774,81,-1,-1));
	vector851.push_back(PointPair(107,222,328,-1,-1));
	vector851.push_back(PointPair(103,263,275,-1,-1));
	vector851.push_back(PointPair(102,317,397,-1,-1));
	vector851.push_back(PointPair(4,713,475,-1,-1));
	vector851.push_back(PointPair(5,981,421,-1,-1));
	vector861.push_back(PointPair(1,722,83,-1,-1));
	vector861.push_back(PointPair(107,142,300,-1,-1));
	vector861.push_back(PointPair(103,246,314,-1,-1));
	vector861.push_back(PointPair(102,289,397,-1,-1));
	vector861.push_back(PointPair(4,605,463,-1,-1));
	vector861.push_back(PointPair(5,890,440,-1,-1));
	vector871.push_back(PointPair(1,601,79,-1,-1));
	vector871.push_back(PointPair(107,86,300,-1,-1));
	vector871.push_back(PointPair(103,270,315,-1,-1));
	vector871.push_back(PointPair(102,273,402,-1,-1));
	vector871.push_back(PointPair(4,558,480,-1,-1));
	vector871.push_back(PointPair(5,727,531,-1,-1));
	vector881.push_back(PointPair(1,578,86,-1,-1));
	vector881.push_back(PointPair(107,85,300,-1,-1));
	vector881.push_back(PointPair(103,322,248,-1,-1));
	vector881.push_back(PointPair(102,302,394,-1,-1));
	vector881.push_back(PointPair(4,535,465,-1,-1));
	vector881.push_back(PointPair(5,602,559,-1,-1));

	basetruth.insert(std::make_pair(1,vector1));
	basetruth.insert(std::make_pair(11,vector11));
	basetruth.insert(std::make_pair(21,vector21));
	basetruth.insert(std::make_pair(31,vector31));
	basetruth.insert(std::make_pair(41,vector41));
	basetruth.insert(std::make_pair(51,vector51));
	basetruth.insert(std::make_pair(61,vector61));
	basetruth.insert(std::make_pair(71,vector71));
	basetruth.insert(std::make_pair(81,vector81));
	basetruth.insert(std::make_pair(91,vector91));
	basetruth.insert(std::make_pair(101,vector101));
	basetruth.insert(std::make_pair(111,vector111));
	basetruth.insert(std::make_pair(121,vector121));
	basetruth.insert(std::make_pair(131,vector131));
	basetruth.insert(std::make_pair(141,vector141));
	basetruth.insert(std::make_pair(151,vector151));
	basetruth.insert(std::make_pair(161,vector161));
	basetruth.insert(std::make_pair(171,vector171));
	basetruth.insert(std::make_pair(181,vector181));
	basetruth.insert(std::make_pair(191,vector191));
	basetruth.insert(std::make_pair(201,vector201));
	basetruth.insert(std::make_pair(211,vector211));
	basetruth.insert(std::make_pair(221,vector221));
	basetruth.insert(std::make_pair(231,vector231));
	basetruth.insert(std::make_pair(241,vector241));
	basetruth.insert(std::make_pair(251,vector251));
	basetruth.insert(std::make_pair(261,vector261));
	basetruth.insert(std::make_pair(271,vector271));
	basetruth.insert(std::make_pair(281,vector281));
	basetruth.insert(std::make_pair(291,vector291));
	basetruth.insert(std::make_pair(301,vector301));
	basetruth.insert(std::make_pair(311,vector311));
	basetruth.insert(std::make_pair(321,vector321));
	basetruth.insert(std::make_pair(331,vector331));
	basetruth.insert(std::make_pair(341,vector341));
	basetruth.insert(std::make_pair(351,vector351));
	basetruth.insert(std::make_pair(361,vector361));
	basetruth.insert(std::make_pair(371,vector371));
	basetruth.insert(std::make_pair(381,vector381));
	basetruth.insert(std::make_pair(391,vector391));
	basetruth.insert(std::make_pair(401,vector401));
	basetruth.insert(std::make_pair(411,vector411));
	basetruth.insert(std::make_pair(421,vector421));
	basetruth.insert(std::make_pair(431,vector431));
	basetruth.insert(std::make_pair(441,vector441));
	basetruth.insert(std::make_pair(451,vector451));
	basetruth.insert(std::make_pair(461,vector461));
	basetruth.insert(std::make_pair(471,vector471));
	basetruth.insert(std::make_pair(481,vector481));
	basetruth.insert(std::make_pair(491,vector491));
	basetruth.insert(std::make_pair(501,vector501));
	basetruth.insert(std::make_pair(511,vector511));
	basetruth.insert(std::make_pair(521,vector521));
	basetruth.insert(std::make_pair(531,vector531));
	basetruth.insert(std::make_pair(541,vector541));
	basetruth.insert(std::make_pair(551,vector551));
	basetruth.insert(std::make_pair(561,vector561));
	basetruth.insert(std::make_pair(571,vector571));
	basetruth.insert(std::make_pair(581,vector581));
	basetruth.insert(std::make_pair(591,vector591));
	basetruth.insert(std::make_pair(601,vector601));
	basetruth.insert(std::make_pair(611,vector611));
	basetruth.insert(std::make_pair(621,vector621));
	basetruth.insert(std::make_pair(631,vector631));
	basetruth.insert(std::make_pair(641,vector641));
	basetruth.insert(std::make_pair(651,vector651));
	basetruth.insert(std::make_pair(661,vector661));
	basetruth.insert(std::make_pair(671,vector671));
	basetruth.insert(std::make_pair(681,vector681));
	basetruth.insert(std::make_pair(691,vector691));
	basetruth.insert(std::make_pair(701,vector701));
	basetruth.insert(std::make_pair(711,vector711));
	basetruth.insert(std::make_pair(721,vector721));
	basetruth.insert(std::make_pair(731,vector731));
	basetruth.insert(std::make_pair(741,vector741));
	basetruth.insert(std::make_pair(751,vector751));
	basetruth.insert(std::make_pair(761,vector761));
	basetruth.insert(std::make_pair(771,vector771));
	basetruth.insert(std::make_pair(781,vector781));
	basetruth.insert(std::make_pair(791,vector791));
	basetruth.insert(std::make_pair(801,vector801));
	basetruth.insert(std::make_pair(811,vector811));
	basetruth.insert(std::make_pair(821,vector821));
	basetruth.insert(std::make_pair(831,vector831));
	basetruth.insert(std::make_pair(841,vector841));
	basetruth.insert(std::make_pair(851,vector851));
	basetruth.insert(std::make_pair(861,vector861));
	basetruth.insert(std::make_pair(871,vector871));
	basetruth.insert(std::make_pair(881,vector881));
}








