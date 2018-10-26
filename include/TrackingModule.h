#include <RecognizedPlayer.h>
#include <ModelImageGenerator.h>
#include <PointPair.h>
#include <iostream>

using namespace std;

class TrackingModule {
	public:
		void handleInput(std::vector<RecognizedPlayer> inputHud, std::vector<RecognizedPlayer> inputMar, std::vector<RecognizedPlayer> inputMic);
	private:
		bool isPossiblySamePlayer(RecognizedPlayer a, RecognizedPlayer b);
};
