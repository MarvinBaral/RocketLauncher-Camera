#include "servoControl.h"
#include <iostream>

ServoControl::ServoControl(QString pPortName) {
	serial = new QSerialPort();
	portName = pPortName;
    degrees[0] = 0;
	degrees[1] = 0;
	initSerial(portName);
	serial->open(QIODevice::ReadWrite);
}

void ServoControl::updateServo(int index, signed int degreeDiff) {
    degrees[index] += degreeDiff;
    if (degrees[index] < MAX_DEGREES[index][0]) {
        degrees[index] = MAX_DEGREES[index][0];
    } else if (degrees[index] > MAX_DEGREES[index][1]) {
        degrees[index] = MAX_DEGREES[index][1];
    }

    this->setServo(index, degrees[index]);
}

void ServoControl::setServo(int index, int degree) {
    if (degree >= MAX_DEGREES[index][0] && degree <= MAX_DEGREES[index][1]) {
        degrees[index] = degree;
        QString command = QString::number(index) + ";" + QString::number(degrees[index]) + ";";
        serial->write(command.toLocal8Bit());
        serial->flush();
        serial->waitForReadyRead(100);
        serial->readAll();
	}
}

void ServoControl::initSerial(const QString &PORT_NAME) {
    serial->setPortName(PORT_NAME);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
}

void ServoControl::shoot(){
    serial->write(QString('s').toLocal8Bit());
	serial->flush();
}

void ServoControl::reset()
{
	serial->close();
	//initSerial(portName); how about this?????? Didn't test with it.
	serial->open(QIODevice::ReadWrite);
}

void ServoControl::printResponse()
{
	serial->waitForReadyRead(10);
	QByteArray response = serial->readAll();
	if (!response.isEmpty() && !response.isNull()) {
		std::cout << response.toStdString();
	}
}

ServoControl::~ServoControl()
{
	serial->close();
}
