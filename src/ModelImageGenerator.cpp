#include <ModelImageGenerator.h>

Mat ModelImageGenerator::createFieldModel(std::string title, std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlack) {
	std::vector<PointPair> emptyVector;
	std::vector<PointPair> emptyVector2;
	return createFieldModel(title, additionalPointsRed, additionalPointsGreen, additionalPointsBlack, emptyVector, emptyVector2);
}

Mat ModelImageGenerator::createFieldModel(std::string title, std::vector<PointPair> additionalPointsRed, std::vector<PointPair> additionalPointsGreen, std::vector<PointPair> additionalPointsBlack, std::vector<PointPair> additionalPointsYellow) {
	std::vector<PointPair> emptyVector;
	return createFieldModel(title, additionalPointsRed, additionalPointsGreen, additionalPointsBlack, additionalPointsYellow, emptyVector);
}

// Creates an image of the Model
Mat ModelImageGenerator::createFieldModel(
	std::string title, 					// Title of window
	std::vector<PointPair> additionalPointsRed, 		// Some red Points
	std::vector<PointPair> additionalPointsGreen, 		// Some green points
	std::vector<PointPair> additionalPointsBlack, 		// Some black points
	std::vector<PointPair> additionalPointsYellow, 		// Will be yellow lines
	std::vector<PointPair> basetruth			// Will be drawn as groundtruth
) {
	Mat field(650,1250, CV_8UC3, Scalar(153,136,119));
	// white field
	int white_x = 141;
	int white_y = 70;
	int white_width = 900;
	int white_height = 450;
	Scalar white_color(255,255,255);
	Point white_topleft(white_x,white_y) ;
	Point white_bottomleft(white_x,white_y+white_height) ;
	Point white_topright(white_x+white_width,white_y) ;
	Point white_bottomright(white_x+white_width,white_y+white_height) ;
	Point white_topthird(white_x+white_width/3,white_y) ;
	Point white_bottomthird(white_x+white_width/3,white_y+white_height) ;
	Point white_tophalf(white_x+white_width/2,white_y) ;
	Point white_bottomhalf(white_x+white_width/2,white_y+white_height) ;
	Point white_topTwoThirds(white_x+2*(white_width/3),white_y) ;
	Point white_bottomTwoThirds(white_x+2*(white_width/3),white_y+white_height) ;
	line(field, white_topleft, white_bottomleft, white_color, 2);
	line(field, white_topleft, white_topright, white_color, 2);
	line(field, white_topright, white_bottomright, white_color, 2);
	line(field, white_bottomleft, white_bottomright, white_color, 2);
	line(field, white_topthird, white_bottomthird, white_color, 2);
	line(field, white_tophalf, white_bottomhalf, white_color, 2);
	line(field, white_topTwoThirds, white_bottomTwoThirds, white_color, 2);
	// yellow field
	int yellow_x = 256;
	int yellow_y = 142;
	int yellow_width = 670;
	int yellow_height = 306;
	Scalar yellow_color(0,255,255);
	Point yellow_topleft(yellow_x,yellow_y) ;
	Point yellow_bottomleft(yellow_x,yellow_y+yellow_height) ;
	Point yellow_topright(yellow_x+yellow_width,yellow_y) ;
	Point yellow_bottomright(yellow_x+yellow_width,yellow_y+yellow_height) ;
	line(field, yellow_topleft, yellow_bottomleft, yellow_color, 2);
	line(field, yellow_topleft, yellow_topright, yellow_color, 2);
	line(field, yellow_topright, yellow_bottomright, yellow_color, 2);
	line(field, yellow_bottomleft, yellow_bottomright, yellow_color, 2);

	// Distance-Reference, shows 2m
	//line(field, Point(50, 20), Point(150, 20), yellow_color, 2);

	for(PointPair& pp : additionalPointsRed) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 8, Scalar(0, 0, 255));
		putText(field, std::to_string(pp.id), cvPoint(pp.p2.x / 2+15,pp.p2.y / 2+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
	}

	for(PointPair& pp : additionalPointsGreen) {
		/* 
		// DEBUG: For Image-Exporting
		if (pp.id == -2)
			line(field, Point(pp.p1.x/2,pp.p1.y/2), Point(pp.p2.x/2,pp.p2.y/2), Scalar (0,0,255), 1);
		else
			line(field, Point(pp.p1.x/2,pp.p1.y/2), Point(pp.p2.x/2,pp.p2.y/2), yellow_color, 1);
		*/
		line(field, Point(pp.p1.x/2,pp.p1.y/2), Point(pp.p2.x/2,pp.p2.y/2), yellow_color, 1);
	}

	for(PointPair& pp : additionalPointsBlack) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 8, Scalar(0, 0, 0));
		putText(field, std::to_string(pp.id), cvPoint(pp.p2.x / 2+15,pp.p2.y / 2+15), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,200,250), 1, CV_AA);
	}

	for(PointPair& pp : additionalPointsYellow) {
		circle(field, Point(pp.p2.x / 2, pp.p2.y / 2), 6, yellow_color);
	}

	for(PointPair& pp : basetruth) {
		circle(field, Point(pp.p1.x, pp.p1.y), 6, Scalar(0, 255, 0));
		putText(field, std::to_string(pp.id), Point(pp.p1.x, pp.p1.y), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,200,250), 1, CV_AA);
	}

	imshow(title, field);
	//imwrite( "model.jpg", field );
	return field;
}
