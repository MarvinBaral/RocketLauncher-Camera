#include <QSerialPort>
#include <QTime>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "servoControl.hpp"
#include "openCV.cpp"

const unsigned short int STEP_DEGREE = 5;
const bool SHOW_RESPONSE_FROM_ARDUINO = false;
const QString PORT_NAME = "/dev/ttyACM0";
long unsigned int frameCount = 0;
unsigned int fpsCount = 0;
QTime startTime;
int keyPressed;

int main(int argc, char* argv[]) {

    QSerialPort* serial = new QSerialPort(); //https://www.youtube.com/watch?v=UD78xyKbrfk
    ServoControl* servoControl = new ServoControl(serial);
    OpenCV* openCV = new OpenCV(servoControl);

    servoControl->initSerial(PORT_NAME);
    serial->open(QIODevice::ReadWrite);

    startTime = QTime::currentTime();
    do {
        //http://docs.opencv.org/2.4/modules/highgui/doc/reading_and_writing_images_and_video.html#videocapture-read
        frameCount++;
        fpsCount++;
        openCV->detectBallByAverage();
        openCV->updateFrame();
        openCV->show();

        keyPressed = cv::waitKey(1);
        switch (keyPressed) {
        case -1: break;
        case 107: //k
            openCV->showColorOfCenteredPixel();
            break;
        case 65361: //left
            servoControl->updateServo(0, -STEP_DEGREE);
            break;
        case 65363: //right
            servoControl->updateServo(0, STEP_DEGREE);
            break;
        case 65362: //up
            servoControl->updateServo(1, -STEP_DEGREE);
            break;
        case 65364: //down
            servoControl->updateServo(1, STEP_DEGREE);
            break;
        case 10: //enter = shoot
            servoControl->shoot();
            break;
        case 114: //r = reset
            serial->close();
            serial->open(QIODevice::ReadWrite);
            break;
        default:
            std::cout << "pressed " << keyPressed << std::endl;
            break;
        }

        if (SHOW_RESPONSE_FROM_ARDUINO) {
            serial->waitForReadyRead(10);
            QByteArray response = serial->readAll();
            if (!response.isEmpty() && !response.isNull()) {
                std::cout << response.toStdString();
            }
        }

        if (startTime <= QTime::currentTime().addSecs(-1)) {
            startTime = QTime::currentTime();
            std::cout << "fps:" << fpsCount << std::endl;
            fpsCount = 0;
        }
    } while (keyPressed != 27);

    serial->close();
    std::cout << "esc key pressed - aborted" << std::endl;

    delete openCV;
    delete servoControl;
    delete serial;

    return 0;
}

//http://rodrigoberriel.com/2014/11/using-opencv-3-qt-creator-3-2-qt-5-3/
