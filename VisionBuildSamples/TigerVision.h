#pragma once
#ifndef _TIGERVISION_H_INCLUDED
#define _TIGERVISION_H_INCLUDED

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <ntcore.h>
#include <networktables/NetworkTable.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <memory>


class TigerVision {
public:
	TigerVision();
	TigerVision(int resizeX, int resizeY);
	cv::Mat FindTarget(cv::Mat input);
	void FilterContours();
	void ShowTarget();
	void DrawCoords(cv::Point pnt);
	float CalculateAngleBetweenCameraAndPixel();
private:
	cv::VideoCapture vidCap;
	cv::Mat imgOriginal, imgResize, imgHSV, imgThreshold, imgContours;
	cv::Size imageSize;
	std::vector<std::vector<cv::Point>> contours, selected;
	std::vector<cv::Point> hull;
	std::vector<cv::Vec4i> hierarchy;
	cv::Point targetTextX, targetTextY, centerPixel, targetCenter;

	const cv::Scalar LOWER_BOUNDS = cv::Scalar(44, 163, 83);
	const cv::Scalar UPPER_BOUNDS = cv::Scalar(80, 255, 255);
	const cv::Scalar RED = cv::Scalar(0, 0, 255);

	const int RECTANCLE_AREA_SIZE = 125;
	const float SOLIDITY_MIN = 0.45;
	const float SOLIDITY_MAX = 1;
	const int ASPECT_RATIO = 1;
	const float CAMERA_FOV = 47;
	const float PI = 3.1415926535897;
	
	const float CameraOffset = 8.375;

	std::ofstream logFile;
	const std::string FILE_EXTENSION = ".jpg";
	std::string finalFileName;
	std::string outputFileName;

	cv::VideoWriter writer;
 
	int centerX;
	int centerY;
	float degreesPerPixel;
	float angleToTarget;
	
	std::shared_ptr<NetworkTable> visionTable;
};

#endif
