#include "TigerVision.h"

TigerVision::TigerVision() {
	imageSize = cv::Size(320,240);
	centerPixel = cv::Point(320 / 2 - .5, 240 / 2);
	NetworkTable::SetClientMode();
	NetworkTable::SetTeam(2053);
	NetworkTable::Initialize();
	visionTable = NetworkTable::GetTable("vision");
}

TigerVision::TigerVision(int resizeX = 320, int resizeY = 240) {
	imageSize = cv::Size(resizeX, resizeY);
	centerPixel = cv::Point(resizeX / 2 - .5, resizeY / 2);
	NetworkTable::SetClientMode();
	NetworkTable::SetTeam(2053);
	NetworkTable::Initialize();
	visionTable = NetworkTable::GetTable("vision");
}

cv::Mat TigerVision::FindTarget(cv::Mat input) {
	//resets 2D array of points for next time through loop
	contours.clear();
	selected.clear();
	
	imgOriginal = input;

	//resizes it to desired Size
	cv::resize(imgOriginal, imgResize, imageSize, 0, 0, cv::INTER_LINEAR);
	//image to save to test
	//cv::imwrite("/home/pi/Pictures/imgResize.jpg", imgResize);
	//converts to HSV color space
	cv::cvtColor(imgResize, imgHSV, cv::COLOR_BGR2HSV);
	//checks for HSV values in range
	cv::inRange(imgHSV, LOWER_BOUNDS, UPPER_BOUNDS, imgThreshold);
	//clones image so we have a copy of the threshold matrix
	imgContours = imgThreshold.clone();
	//finds closed shapes within threshold image
	cv::findContours(imgContours, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	TigerVision::FilterContours();
	TigerVision::ShowTarget();

	std::vector<cv::Point> midPointsOfSelected;

	if (selected.size() == 2) {
		float aspect;
		for (int i = 0; i < selected.size(); i++) {
			cv::Rect targetRectangle = cv::boundingRect(selected[i]);
			aspect = (float)targetRectangle.width / (float)targetRectangle.height;
			centerX = targetRectangle.br().x - targetRectangle.width / 2;
			centerY = targetRectangle.br().y - targetRectangle.height / 2;
			targetCenter = cv::Point(centerX, centerY);
			midPointsOfSelected.push_back(targetCenter);
			angleToTarget = TigerVision::CalculateAngleBetweenCameraAndPixel();
		}
		//finds midpoint of two rectangles
		cv::Point midPoint = (midPointsOfSelected[0] + midPointsOfSelected[1]) * 0.5;
		
		//visionTable->PutNumber("centerX", midPoint.x);
		//visionTable->PutNumber("centerY", midPoint.y);
		
		cv::line(imgResize, centerPixel, midPoint, RED);
		cv::circle(imgResize, midPoint, 3, RED);
		if (aspect < 1) {
			cv::putText(imgResize, "Gear", cv::Point(0, 45), cv::FONT_HERSHEY_PLAIN, 1, RED);
		}
		else {
			cv::putText(imgResize, "Boiler", cv::Point(0, 45), cv::FONT_HERSHEY_PLAIN, 1, RED);
		}
		TigerVision::DrawCoords(midPoint);
	}
	return imgResize;
}

void TigerVision::FilterContours() {
	//filters contours on aspect ration and min area and solidity
	for (int i = 0; i < contours.size(); i++) {
		cv::Rect rect = boundingRect(contours[i]);
		float aspect = (float)rect.width / (float)rect.height;

		//does solidity calculations
		convexHull(contours[i], hull);
		float area = contourArea(contours[i]);
		float hull_area = contourArea(hull);
		float solidity = (float)area / hull_area;

		if (/*aspect > ASPECT_RATIO &&*/ rect.area() > RECTANCLE_AREA_SIZE && (solidity >= SOLIDITY_MIN && solidity <= SOLIDITY_MAX)) {
			selected.push_back(contours[i]);
		}
	}
}

void TigerVision::ShowTarget() {
	//draw rectangles on selected contours
	for (int i = 0; i < selected.size(); i++) {
		cv::Rect rect = cv::boundingRect(selected[i]);
		cv::rectangle(imgResize, rect.br(), rect.tl(), RED);
	}
}

void TigerVision::DrawCoords(cv::Point pnt) {
	targetTextX = cv::Point(0, 15);
	targetTextY = cv::Point(0, 30);
	putText(imgResize, "x: " + std::to_string(pnt.x), targetTextX, cv::FONT_HERSHEY_PLAIN, 1, RED);
	putText(imgResize, "y: " + std::to_string(pnt.y), targetTextY, cv::FONT_HERSHEY_PLAIN, 1, RED);;
}

float TigerVision::CalculateAngleBetweenCameraAndPixel() {
	float focalLengthPixels = .5 * imageSize.width / std::tan((CAMERA_FOV * (PI / 180)) / 2);
	float angle = std::atan((targetCenter.x - centerPixel.x) / focalLengthPixels);
	float angleDegrees = angle * (180 / PI);
	return angleDegrees;
}
