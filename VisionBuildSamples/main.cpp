#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include "TigerVision.h"
#include <cscore.h>




cv::Mat processImage(cv::Mat input) {
	return input;
}

void VisionThread() {
	TigerVision tigerVision;

	cs::UsbCamera camera = cs::UsbCamera("cam0", 0);
	cs::CvSink sink = cs::CvSink("sink");
	sink.SetSource(camera);
	cs::CvSource source = cs::CvSource("src", cs::VideoMode::PixelFormat::kMJPEG, 320, 240, 30);
	cs::MjpegServer mjpegServer = cs::MjpegServer("server", 1185);
	mjpegServer.SetSource(source);
	
	cv::Mat input;
	cv::Mat output;
	
	while(true) {
		auto frameTime = sink.GrabFrame(input);
		if(frameTime == 0)
		{
			continue;
		}
		
		output = tigerVision.FindTarget(input);
		source.PutFrame(output);
	}
}

int main(int argc, const char** argv) {
	VisionThread();
	
	return 0;
}
