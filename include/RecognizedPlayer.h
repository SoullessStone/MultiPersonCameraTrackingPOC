#include "opencv2/core.hpp"
using namespace cv;

struct RecognizedPlayer
{
	private:
		// Information could be unavailable or unclear
		bool containsValidPositionInModel = false;
		bool containsValidIsRed = false;
		bool containsValidShirtNumber = false;

		// The real information
		Point positionInModel;
		bool isRed;
		int shirtNumber;
	public:
		// Constructor
		RecognizedPlayer();

		// Getter and Setter and Validators
		void setPositionInModel(Point positionInModel, bool valid);
		void setIsRed(bool isRed, bool valid);
		void setShirtNumber(int shirtNumber, bool valid);

		bool isPositionInModelValid();
		bool isIsRedValid();
		bool isShirtNumberValid();

		Point getPositionInModel();
		bool getIsRed();
		int getShirtNumber();
		
};
