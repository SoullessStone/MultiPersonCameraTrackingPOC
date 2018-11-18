#include <RecognizedPlayer.h>
#include <ModelImageGenerator.h>
#include <Logger.h>
#include <PointPair.h>
#include <iostream>
#include <map>
#include <algorithm>

using namespace std;

class TrackingModule {
	public:
		void handleInput(int frameId, std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
	private:
		std::map<int, std::vector<RecognizedPlayer>> history;
		std::map<int, int> lastUpdatedPlayer;
		std::vector<RecognizedPlayer> lostPlayers;

		void printHistory();
		bool isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold);
		int getDistance(RecognizedPlayer a, RecognizedPlayer b);
		std::vector<RecognizedPlayer> getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		std::vector<RecognizedPlayer> getFullInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		void createHistory(std::vector<RecognizedPlayer> &curFrameInput, std::vector<RecognizedPlayer> &newHistoryInput, RecognizedPlayer histPlayer, std::vector<PointPair> &notChangedPlayersToDraw, std::vector<PointPair> &redPlayersToDraw, std::vector<PointPair> &blackPlayersToDraw, std::vector<PointPair> &playerMovement);
};
