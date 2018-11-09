// Nötig, weil sonst Klasse doppelt da ist: http://forums.devshed.com/programming-42/compile-error-redefinition-class-437198.html
#ifndef DO_NOT_DEFINE_RECOGNIZEDPLAYER_MULTIPLE_TIMES

	#define DO_NOT_DEFINE_RECOGNIZEDPLAYER_MULTIPLE_TIMES 
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
			int shirtNumber = -1;

			// Information used by the Cameras, not to be used in TrackingModule
			Point positionInPerspective;

			// Debug information
			int camerasPlayerId;
		public:
			// Constructor
			RecognizedPlayer();

			// Getter and Setter and Validators
			void setPositionInPerspective(Point positionInPerspective);
			void setPositionInModel(Point positionInModel, bool valid);
			void setIsRed(bool isRed, bool valid);
			void setShirtNumber(int shirtNumber, bool valid);
			void setCamerasPlayerId(int id);

			bool isPositionInModelValid();
			bool isIsRedValid();
			bool isShirtNumberValid();

			Point getPositionInPerspective();
			Point getPositionInModel();
			bool getIsRed();
			int getShirtNumber();
			int getCamerasPlayerId();
		
			std::string toString();
	};

#endif
