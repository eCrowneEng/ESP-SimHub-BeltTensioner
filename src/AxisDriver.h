#pragma once

#include "./FastAccelStepper.h"
//#include "AVRStepperPins.h" // Only required for AVR controllers

struct AxisSettings {
public:
	int hallSensorPin;
	int pulsePin;
	int directionPin;
	int enablePin;
	int totalWorkingRange;
	int effectiveRange;
	bool reverseDirection;
	int stepperId;
	int centerPosition;
	int centerPositionPercent;
	int maxSpeed = 20000;
	int acceleration = 80000;
	int sensorTriggerLevel = 300;
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="_motorId"></param>
	/// <param name="_hallSensorPin"></param>
	/// <param name="_pulsePin"></param>
	/// <param name="_dirPin"></param>
	/// <param name="_enablePin"></param>
	/// <param name="_workingRange"></param>
	/// <param name="_reverseDirection"></param>
	AxisSettings(int _stepperId, int _hallSensorPin, int _pulsePin, int _directionPin, int _enablePin, int _totalWorkingRange, bool _reverseDirection, int _centerPositionPercent, int _sensorTriggerLevel) {
		stepperId = _stepperId;
		hallSensorPin = _hallSensorPin;
		pulsePin = _pulsePin;
		directionPin = _directionPin;
		enablePin = _enablePin;
		totalWorkingRange = _totalWorkingRange;
		reverseDirection = _reverseDirection;
		centerPositionPercent = _centerPositionPercent;
		centerPosition = (float)(totalWorkingRange * (float)_centerPositionPercent / (float)100);
		effectiveRange = (float)totalWorkingRange * max((float)_centerPositionPercent, (float)100 - (float)_centerPositionPercent) / (float)100;
		sensorTriggerLevel = _sensorTriggerLevel;
	}

	/// <summary>
	/// Converts from 0 - 10000 (5000 being the middle) to 0 - working range;
	/// </summary>
	/// <param name="inputPosition">0-10000 position</param>
	/// <returns>Translated position</returns>
	int convertPosition(int inputPosition) {
		float tmp = (float)constrain((float)inputPosition / (float)32768, (float)-1, (float)1);

		tmp = constrain((float)centerPosition + tmp * (float)effectiveRange, (float)0, (float)totalWorkingRange);
		return tmp;
	}
};

struct AxisState {
	bool motorEnabled = false;
	int targetPosition = -1;
	unsigned long lastChange = 0;
	int queuedPosition = -1;
};

class AxisDriver {
private:

	FastAccelStepper* stepper = NULL;
	AxisSettings* axisSettings;
	AxisState axisState = AxisState();

	int ReadHallSensor() {
		int val = analogRead(axisSettings->hallSensorPin);
		val += analogRead(axisSettings->hallSensorPin);
		val += analogRead(axisSettings->hallSensorPin);
		val = val / 3;
		return abs(val - 512);
	}

	void WaitForStepperStop() {
		delay(5);
		while (stepper->isRunning()) {
			delay(5);
		}
	}

	void PrintLog(String message) {
		//Serial.println("M" + String(axisSettings->stepperId) + " " + message);
	}

	void EnableMotor() {
		if (!axisState.motorEnabled) {
			PrintLog("Enabling motor M");
			digitalWrite(axisSettings->enablePin, HIGH);
			axisState.motorEnabled = true;
			axisState.targetPosition = -1;
			Calibrate();
			axisState.lastChange = millis();
		}
	}

	void DisableMotor() {
		if (axisState.motorEnabled) {
			PrintLog("Disabling motor");
			digitalWrite(axisSettings->enablePin, LOW);
			axisState.motorEnabled = false;
			axisState.targetPosition = -1;
		}
	}

	void calibrationErrorDeath(String message) {
		// Intentionnal dead end
		while (true) {
			DisableMotor();
			PrintLog(message);
			delay(30000);
		}
	}

	void Calibrate() {
		bool calibrationSuccessful = false;
		PrintLog("Starting stepper calibration");
		stepper->setSpeedInHz(4000);		// 500 steps/s
		stepper->setAcceleration(60000);	// 100 steps/s
		stepper->setCurrentPosition(0);
		stepper->moveTo(2000);

		WaitForStepperStop();

		PrintLog("Starting calibration");

		stepper->moveTo(-axisSettings->totalWorkingRange * 3);
		delay(10);

		int val = ReadHallSensor();

		int lastval = 0;
		if (val < axisSettings->sensorTriggerLevel) {
			while (stepper->isRunning()) {
				if (val > axisSettings->sensorTriggerLevel) {
					PrintLog("Sensor triggered");
					stepper->stopMove();
					WaitForStepperStop();
					stepper->setCurrentPosition(-500);
					stepper->moveTo(axisSettings->centerPosition);
					WaitForStepperStop();
					calibrationSuccessful = true;
					break;
				}
				else {
					if (abs(lastval - val) > 20) {
						lastval = val;
					}
					val = ReadHallSensor();
				}
			}
		}
		else {
			calibrationErrorDeath("Calibration ERROR, hall sensor is already sensing the lever, please check hardware and reboot device.");
		}

		if (calibrationSuccessful == false) {
			calibrationErrorDeath("Calibration ERROR, lever not found, please check hardware and reboot device.");
		}

		PrintLog("Calibration succesful.");

		setSpeedSettings();
	}

	void setSpeedSettings() {
		stepper->setSpeedInHz(axisSettings->maxSpeed);	// 25000 steps/s
		stepper->setAcceleration(80000);				// 80000 steps/s
	}

	int ToSignedInt(String s) {
		if (s.length() > 0) {
			int sign = 1;
			if (s.charAt(0) == '-') {
				sign = -1;
				s = s.substring(1);
			}
			return sign * s.toInt();
		}
	}

public:

	String CommandPrefix() {
		return "M" + String(axisSettings->stepperId) + " ";
	}

	void begin(FastAccelStepperEngine* engine, AxisSettings* _axisSettings) {
		axisSettings = _axisSettings;
		pinMode(axisSettings->hallSensorPin, INPUT);
		pinMode(axisSettings->enablePin, OUTPUT);
		pinMode(axisSettings->directionPin, OUTPUT);

		stepper = engine->stepperConnectToPin(axisSettings->pulsePin);

		if (stepper) {
			stepper->setDirectionPin(axisSettings->directionPin, axisSettings->reverseDirection);
			stepper->setSpeedInHz(25000);		// 500 steps/s
			stepper->setAcceleration(15000);	// 100 steps/s
		}

		EnableMotor();
	}

	void processCommand(String command) {

		if (command.startsWith("speed ")) {
			axisSettings->maxSpeed = command.substring(6).toInt();
			PrintLog("Speed set to " + String(axisSettings->maxSpeed));
			setSpeedSettings();
		}
		else {

			int newPosition = axisSettings->convertPosition(command.toInt());

			axisState.lastChange = millis();

			if (axisState.targetPosition != newPosition) {
				EnableMotor();

				axisState.queuedPosition = -1;

				axisState.targetPosition = newPosition;

				// Motor don't move go to position
				if (!stepper->isRunning()) {
					stepper->moveTo(newPosition);
				}
				// If motor is already running in the correct direction and have not passed the target position, set it as new target position
				else if (stepper->getCurrentPosition() < stepper->targetPos() && stepper->getCurrentPosition() < axisState.targetPosition) {
					stepper->moveTo(newPosition);
				}
				// If motor is already running in the correct direction and have not passed the target position, set it as new target position
				else if (stepper->getCurrentPosition() > stepper->targetPos() && stepper->getCurrentPosition() > axisState.targetPosition) {
					stepper->moveTo(newPosition);
				}
				// If motor is not in the correction direction, initiate stop and put the new position in "queue" to be taken into account when the motor will be stopped (see loop).
				else {
					stepper->stopMove();
					axisState.queuedPosition = axisState.targetPosition;
				}
			}
		}
	}

	void loop() {
		// Process queued position as soon as motor is stopped.
		if (axisState.queuedPosition != -1 && !stepper->isRunning()) {
			stepper->moveTo(axisState.queuedPosition);
			axisState.queuedPosition = -1;
		}

		// Automatic motor disable after 30s of inactivity
		if (millis() - axisState.lastChange > 30000) {
			DisableMotor();
		}
	}
};
