#include <RecognizedPlayer.h>
#include <ModelImageGenerator.h>
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

		void printHistory();
		bool isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold);
		std::vector<RecognizedPlayer> getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		void createHistory(std::vector<RecognizedPlayer> &curFrameInput, std::vector<RecognizedPlayer> &newHistoryInput, RecognizedPlayer histPlayer, std::vector<PointPair> &notChangedPlayersToDraw, std::vector<PointPair> &changedPlayersToDraw, std::vector<PointPair> &playerMovement);
};
