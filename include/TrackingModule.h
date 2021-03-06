#include <RecognizedPlayer.h>
#include <ModelImageGenerator.h>
#include <Logger.h>
#include <PointPair.h>
#include <iostream>
#include <map>
#include <algorithm>
// For Distance-Export
//#include <math.h> 

using namespace std;

class TrackingModule {
	public:
		// Constructor
		TrackingModule();

		void handleInput(int frameId, std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		std::vector<int> getHistoryPlayerIds();
		void applyCorrection(int playerId, int frameId, Point newPosition);
		void createVideo();
	private:
		std::map<int, std::vector<RecognizedPlayer>> history;
		std::map<int, int> lastUpdatedPlayer;
		std::vector<RecognizedPlayer> lostPlayers;
		std::vector<Mat> trackingResult;

		void printHistory();
		bool isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold);
		int getDistance(RecognizedPlayer a, RecognizedPlayer b);
		std::vector<RecognizedPlayer> getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		std::vector<RecognizedPlayer> getFullInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
		void createHistory(std::vector<RecognizedPlayer> &curFrameInput, std::vector<RecognizedPlayer> &newHistoryInput, RecognizedPlayer histPlayer, std::vector<PointPair> &notChangedPlayersToDraw, std::vector<PointPair> &redPlayersToDraw, std::vector<PointPair> &blackPlayersToDraw, std::vector<PointPair> &playerMovement);

		// Debug
		std::map<int, std::vector<PointPair>> basetruth;
		void initBasetruth();
};
