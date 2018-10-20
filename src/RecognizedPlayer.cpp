#include <RecognizedPlayer.h>

RecognizedPlayer::RecognizedPlayer()
{
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

bool RecognizedPlayer::getIsRed()
{
	return this->isRed;
}

int RecognizedPlayer::getShirtNumber()
{
	return this->shirtNumber;
}





