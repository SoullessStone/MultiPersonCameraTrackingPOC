#include <RecognizedPlayer.h>

RecognizedPlayer::RecognizedPlayer()
{
}

void RecognizedPlayer::setPositionInPerspective(Point positionInPerspective)
{
	this->positionInPerspective = positionInPerspective;
}

void RecognizedPlayer::setPositionInModel(Point positionInModel, bool valid)
{
	this->positionInModel = positionInModel;
	this->containsValidPositionInModel = valid;
}

void RecognizedPlayer::setIsRed(bool isRed, bool valid)
{
	this->isRed = isRed;
	this->containsValidIsRed = valid;
}

void RecognizedPlayer::setShirtNumber(int shirtNumber, bool valid)
{
	this->shirtNumber = shirtNumber;
	this->containsValidShirtNumber = valid;
}

void RecognizedPlayer::setCamerasPlayerId(int id)
{
	this->camerasPlayerId = id;
}

bool RecognizedPlayer::isPositionInModelValid()
{
	return this->containsValidPositionInModel;
}

bool RecognizedPlayer::isIsRedValid()
{
	return this->containsValidIsRed;
}

bool RecognizedPlayer::isShirtNumberValid()
{
	return this->containsValidShirtNumber;
}

Point RecognizedPlayer::getPositionInModel()
{
	return this->positionInModel;
}

Point RecognizedPlayer::getPositionInPerspective()
{
	return this->positionInPerspective;
}

bool RecognizedPlayer::getIsRed()
{
	return this->isRed;
}

int RecognizedPlayer::getShirtNumber()
{
	return this->shirtNumber;
}

int RecognizedPlayer::getCamerasPlayerId()
{
	return this->camerasPlayerId;
}

std::string RecognizedPlayer::toString()
{
	return "{camerasPlayerId: " + std::to_string(camerasPlayerId) + ", positionInModel: {x: " + std::to_string(positionInModel.x) + ", y: " + std::to_string(positionInModel.y) + "}, positionInPerspective: {x: " + std::to_string(positionInPerspective.x) + ", y: " + std::to_string(positionInPerspective.y) + "}, isRed: " + std::to_string(isRed) + ", shirtNumber: " + std::to_string(shirtNumber) + ", containsValidPositionInModel: " + std::to_string(containsValidPositionInModel) + ", containsValidIsRed: " + std::to_string(containsValidIsRed) + ", containsValidShirtNumber: " + std::to_string(containsValidShirtNumber) + "}";
}



