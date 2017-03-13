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

	float aspect; //aspect ratio
	float widthAvg; // Average of two rectangle widths
	int correctedMidx; //Midpoint adjusted for camera offset
					   //The correction is scaled assuming the entire
					   //target width is visible. Otherwise the
					   //correction will be too little, but should
					   //become more accurate as more of the target
					   //comes into view.
	cv::Point midPoint; //Midpoint of two rectangles
	cv::Rect targetRectangle; //A rectangle work variable
	int badone; //Rectangle to ignore, in the case when 3 are seen
	float aspectA, aspectB; //Temp variables for case 3
	cv::Rect targetRectA, targetRectB; //Temp variables for case 3

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

	switch(selected.size()) {
		case 1:
//			std::cout << "One object found" << std::endl;
			targetRectangle = cv::boundingRect(selected[0]);
			aspect = (float) targetRectangle.width / (float) targetRectangle.height;
			centerX = targetRectangle.br().x - targetRectangle.width / 2;
			centerY = targetRectangle.br().y - targetRectangle.height / 2;
			if(aspect < 1) {
				if(centerX > 160) { //Target to right
					//2.0625 - 4.125 / 2 distance in inches to midway between rectangles
					midPoint.x = centerX + 2.0625 * targetRectangle.width;
				}
				else { //Target to left
					midPoint.x = centerX - 2.0625 * targetRectangle.width;
				}
				//Note that we might have correctedMidx < 0 or > 320.
				//That just means our aim point is too far off to one side to see.
				correctedMidx = midPoint.x - (CameraOffset * targetRectangle.width) / 2;
				midPoint.y = centerY;
				visionTable->PutNumber("centerX", correctedMidx);
				visionTable->PutNumber("centerY", midPoint.y);
			}
			else { //Just aim for the corrected center of rectangle
				correctedMidx = centerX - (CameraOffset * targetRectangle.width) / 15;
				visionTable->PutNumber("centerX", correctedMidx);
				visionTable->PutNumber("centerY", midPoint.y);
			}
			break;
		case 2:
//			std::cout << "Two object found" << std::endl;
			widthAvg = 0.0;
			float aspect;
			for (int i = 0; i < selected.size(); i++) {
				cv::Rect targetRectangle = cv::boundingRect(selected[i]);
				aspect = (float)targetRectangle.width / (float)targetRectangle.height;
				centerX = targetRectangle.br().x - targetRectangle.width / 2;
				centerY = targetRectangle.br().y - targetRectangle.height / 2;
				widthAvg += targetRectangle.width / 2;
				targetCenter = cv::Point(centerX, centerY);
				midPointsOfSelected.push_back(targetCenter);
				angleToTarget = TigerVision::CalculateAngleBetweenCameraAndPixel();
			}
			//finds midpoint of two rectangles
			midPoint = (midPointsOfSelected[0] + midPointsOfSelected[1]) * 0.5;
			
			if(aspect < 1) {
				correctedMidx = midPoint.x - CameraOffset * widthAvg / 2;
			}
			else {
				correctedMidx = midPoint.x - CameraOffset * widthAvg / 15;
			}

			visionTable->PutNumber("centerX", correctedMidx);
			visionTable->PutNumber("centerY", midPoint.y);
			
			//ADDED ON 3/13/17
			cv::Rect leftRect = cv::boundingRect(selected[0]);
			cv::Rect leftRect = cv::boundingRect(selected[1]);
			int distanceToCenterLeft = (320/2) - (leftRect.br().x - leftRect.width / 2);
			int distanceToCenterRight = (320/2) - (rightRect.br().x - rightRect.width / 2);
			
			visionTable->PutNumber("LeftDistanceToCenter", distanceToCenterLeft);
			visionTable->PutNumber("RightDistanceToCenter", distanceToCenterRight);

			cv::line(imgResize, centerPixel, midPoint, RED);
			cv::circle(imgResize, midPoint, 3, RED);
			if (aspect < 1) {
				cv::putText(imgResize, "Gear", cv::Point(0, 45), cv::FONT_HERSHEY_PLAIN, 1, RED);
			}
			else {
				cv::putText(imgResize, "Boiler", cv::Point(0, 45), cv::FONT_HERSHEY_PLAIN, 1, RED);
			}
			TigerVision::DrawCoords(midPoint);
			break;
		case 3:
			std::cout << "Three object found" << std::endl;
			/* The general idea in the case of seeing three rectangles is that  */
			/* we need to throw out one.  If looking at the gear target, the    */
			/* two "good" ones should have an aspect ratio less than one, their */
			/* bottoms should be at the same height, and they should be the     */
			/* same height.  For the boiler, we ignore the aspect ratio because */
			/* if we see just a small section it might be less than one.  But   */
			/* the "good" pair should have identical widths and left sides.     */
			//We choose "within 5 pixels to define "same".
			badone = -1; // -1 indicates not found
			for(int i = 0; (i < 3) && (badone == -1); i++) {
				targetRectA = cv::boundingRect(selected[(i + 1) % 3]);
				targetRectB = cv::boundingRect(selected[(i + 2) % 3]);
				aspectA = (float) targetRectA.width / (float) targetRectA.height;
				aspectB = (float) targetRectB.width / (float) targetRectB.height;
				if((aspectA < 1) && (aspectB < 1) && (abs(targetRectA.br().y - targetRectB.br().y) < 6) && (abs(targetRectA.width - targetRectB.width) < 6)) {
					badone = i;
					std::cout << "bad gear: " << i << std::endl;

				}
			}
			for(int i = 0; (i < 3) && (badone == -1); i++)  {  // boiler possibility
   				targetRectA = cv::boundingRect(selected[(i+1) % 3]);
   				targetRectB = cv::boundingRect(selected[(i+2) % 3]);
   				if ((abs(targetRectA.br().x - targetRectB.br().x) < 6) && (abs(targetRectA.width - targetRectB.width) < 6)) {
					badone = i;
					std::cout << "bad boiler: " << i << std::endl;
				}
  			}
			/* Now, if we have found a rectangle to ignore, do the same as */
 			/* for case 2 with the remaining two rectangles.               */
  			if (badone != -1)  {
				std::cout << "got a bad one: " << badone << std::endl;
				widthAvg = 0.0;
				for (int i = 0; i < selected.size(); i++) {
					if (i != badone) {  // Don't use the bad one
						std::cout << "use me: " << i << std::endl;
						targetRectangle = cv::boundingRect(selected[i]);
						aspect = (float)targetRectangle.width / (float)targetRectangle.height;
						centerX = targetRectangle.br().x - targetRectangle.width / 2;
						centerY = targetRectangle.br().y - targetRectangle.height / 2;
						widthAvg += targetRectangle.width / 2;
						targetCenter = cv::Point(centerX, centerY);
						midPointsOfSelected.push_back(targetCenter);
						angleToTarget = TigerVision::CalculateAngleBetweenCameraAndPixel();
					}
				}
				//finds midpoint of two rectangles
				midPoint = (midPointsOfSelected[0] + midPointsOfSelected[1]) * 0.5;

				if (aspect < 1) {  // found "Gear" target
					correctedMidx = midPoint.x - CameraOffset*widthAvg/2;
					std::cout << "got a gear maybe " << std::endl;
				} 
				else {  // found "Boiler" target
					correctedMidx = midPoint.x - CameraOffset*widthAvg/15;
					std::cout << "got a boiler maybe " << std::endl;
				}
				
				
				//ADDED ON 3/13/17
				cv::Rect leftRect = cv::boundingRect(selected[0]);
				cv::Rect leftRect = cv::boundingRect(selected[1]);
				int distanceToCenterLeft = (320/2) - (leftRect.br().x - leftRect.width / 2);
				int distanceToCenterRight = (320/2) - (rightRect.br().x - rightRect.width / 2);
			
				visionTable->PutNumber("LeftDistanceToCenter", distanceToCenterLeft);
				visionTable->PutNumber("RightDistanceToCenter", distanceToCenterRight);
				
				visionTable->PutNumber("centerX", correctedMidx);
				visionTable->PutNumber("centerY", midPoint.y);

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
			break;
		default:
//			std::cout << "none/too many object found" << std::endl;
			break;
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
