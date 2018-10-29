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

		void printHistory();
		bool isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b, int threshold);
		std::vector<RecognizedPlayer> getMergedInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
};
