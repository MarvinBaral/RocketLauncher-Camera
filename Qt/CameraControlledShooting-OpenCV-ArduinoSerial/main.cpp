#include <QSerialPort>
#include <QTime>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <queue>
#include <thread>
#include <chrono>
#include "servoControl.h"
#include "missionControlCenter.h"
#include "main.h"

std::queue<Position> positions;
QTime timer;
std::mutex cv_gui;
std::mutex pos_queue;
unsigned int fpsCount = 0;
const short HARDWARE_VERSION = V1_1;
const float PI = 3.14159265359;
bool automaticMode = true;
bool displayWindow = true;
bool recordPosition = true;

int main() {
	const unsigned short int STEP_DEGREE = 5;
	const bool SHOW_RESPONSE_FROM_ARDUINO = false;
	const QString PORT_NAME = "/dev/ttyACM1";
	const bool SHOW_FPS = true;
	QTime fpsTimer;
	fpsTimer.start();
	int keyPressed;
	std::string windowTitle = "Abschusskamera";
	const short USB_CAM = 2;
	cv::VideoCapture* capture = new cv::VideoCapture(USB_CAM);
	if (!capture->isOpened()) {
		std::cout << "Cannot open the video cam. Please connect the USB-Cam!" << std::endl;
	}
	std::cout << "Theoretically possible fps:" << capture->get(CV_CAP_PROP_FPS) << std::endl;
	if (displayWindow) {
		cv::namedWindow(windowTitle, CV_WINDOW_AUTOSIZE);
	}
	ServoControl* servoControl = new ServoControl(PORT_NAME);
	MissionControlCenter* missionControlCenter = new MissionControlCenter(servoControl, windowTitle, capture);

	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //main thread doesn't do a lot, its better to give some time to the worker threads
		missionControlCenter->handleShooting();
		cv_gui.lock();
		keyPressed = cv::waitKey(1);
		cv_gui.unlock();
        switch (keyPressed) {
        case -1: break;
		case 97: //a = automatic mode
			automaticMode = true;
			recordPosition = true;
			break;
		case 109: //m = manual mode
			automaticMode = false;
			recordPosition = false;
			break;
		case 99: //c = clear
			missionControlCenter->allowedToShoot = true;
			recordPosition = true;
			break;
        case 108: //l = lock
			missionControlCenter->allowedToShoot = false;
			recordPosition = false;
            break;
		case 81: //left
			if (!automaticMode)
				servoControl->updateServo(0, -STEP_DEGREE);
            break;
		case 83: //right
			if (!automaticMode)
				servoControl->updateServo(0, STEP_DEGREE);
            break;
		case 82: //up
			if (!automaticMode)
				servoControl->updateServo(1, -STEP_DEGREE);
            break;
		case 84: //down
			if (!automaticMode)
				servoControl->updateServo(1, STEP_DEGREE);
            break;
        case 10: //enter = shoot
            servoControl->shoot();
            break;
        case 114: //r = reset
			servoControl->reset();
            break;
        default:
#ifdef DEBUG
			std::cout << "pressed " << keyPressed << std::endl;
#endif
			break;
        }

		if (SHOW_RESPONSE_FROM_ARDUINO) {
			servoControl->printResponse();
        }

		if (SHOW_FPS && fpsTimer.elapsed() >= 1000) {
			std::cout << "fps:" << ((fpsCount * 1000.f)/fpsTimer.elapsed())  << std::endl;
			fpsTimer.restart();
			fpsCount = 0;
        }
    } while (keyPressed != 27);

    std::cout << "esc key pressed - aborted" << std::endl;


	delete servoControl;
	delete missionControlCenter;

    return 0;
}
