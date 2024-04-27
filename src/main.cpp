#include <Arduino.h>
#include <EspSimHub.h>
#include "./AxisDriver.h"

// No longer have to define whether it's an ESP32 or ESP8266, just do an initial compilation and
// VSCode will pick  up the right environment from platformio.ini

#define INCLUDE_WIFI true
// Less secure if you plan to commit or share your files, but saves a bunch of memory. 
// If you hardcode credentials the device will only work in your network
#define USE_HARDCODED_CREDENTIALS false

#if INCLUDE_WIFI
#if USE_HARDCODED_CREDENTIALS
  #define WIFI_SSID "SSID"
  #define WIFI_PASSWORD "PASSWORD"
#endif

  #define BRIDGE_PORT 10001 // Perle TruePort uses port 10,001 for the first serial routed to the client
  #define DEBUG_TCP_BRIDGE false // Emits extra events to Serial that show network communication, set to false to save memory and make faster

  #include <TcpSerialBridge2.h>
  #include <ECrowneWifi.h>
  #include <FullLoopbackStream.h>

  FullLoopbackStream outgoingStream;
  FullLoopbackStream incomingStream;

  #define StreamReadStringUntil incomingStream.readStringUntil
  #define StreamAvailable incomingStream.available
  #define StreamPrintln outgoingStream.println

#else
  #define StreamReadStringUntil Serial.readStringUntil
  #define StreamAvailable Serial.available
  #define StreamPrintln Serial.println
#endif // INCLUDE_WIFI

// GLOBAL SETTINGS : How many steps maximum the stepper can travel
const int totalWorkingRange = 2000;
const int centerPositionPercent = 50;

// Sensor sensitivity lower values will make it more sensitive, higher less sensitive (default 100)
const int sensorTriggerLevel = 300;

// STEPPER 1 SETTINGS
const int stepper1_pulsePin = 21;
const int stepper1_directionPin = 22;
const int stepper1_enablePin = 23;
const int stepper1_hallSensorPin = 36; // Must be an analog pin
const int stepper1_reverseDirection = false;

// STEPPER 2 SETTINGS
#define ENABLE_STEPPER_2;
#if defined(ENABLE_STEPPER_2)
  const int stepper2_pulsePin = 25;
  const int stepper2_directionPin = 26;
  const int stepper2_enablePin = 27;
  const int stepper2_hallSensorPin = 39; // Must be an analog pin
  const int stepper2_reverseDirection = true;
#endif

FastAccelStepperEngine engine = FastAccelStepperEngine();

AxisDriver axisDriver1 = AxisDriver();
AxisSettings axisSettings1 = AxisSettings(
  1,
  stepper1_hallSensorPin,
  stepper1_pulsePin,
  stepper1_directionPin,
  stepper1_enablePin,
  totalWorkingRange,
  stepper1_reverseDirection,
  centerPositionPercent,
  sensorTriggerLevel);

#if defined(ENABLE_STEPPER_2)
  AxisDriver axisDriver2 = AxisDriver();
  AxisSettings axisSettings2 = AxisSettings(
	  2,
	  stepper2_hallSensorPin,
	  stepper2_pulsePin,
	  stepper2_directionPin,
	  stepper2_enablePin,
	  totalWorkingRange,
	  stepper2_reverseDirection,
	  centerPositionPercent,
	  sensorTriggerLevel);
#endif

AxisDriver* axisDrivers[] = {
   &axisDriver1
#if defined(ENABLE_STEPPER_2)
	,&axisDriver2
#endif
};

AxisSettings* axisSettings[] = {
   &axisSettings1
#if defined(ENABLE_STEPPER_2)
	,&axisSettings2
#endif
};

const int axisDriversCount = sizeof(axisDrivers) / sizeof(AxisDriver*);

void processCommand(String command) {
	bool processed = false;
	for (int i = 0; i < axisDriversCount; i++) {
		if (command.startsWith(axisDrivers[i]->CommandPrefix())) {
			axisDrivers[i]->processCommand(command.substring(3));
			processed = true;
		}
	}

	if (!processed) {
		outgoingStream.println("Unrecognised command");
	}
}

void setup() {
  #if INCLUDE_WIFI
    Serial.begin(115200);
  #else
    Serial.begin(19200);
  #endif
  // StreamPrintln(String(axisDriversCount) + " steppers enabled");

  #if INCLUDE_WIFI
    ECrowneWifi::setup(&outgoingStream, &incomingStream);
  #endif

  #ifdef ESP32
    analogReadResolution(8); // Set the ADC to 8 bits resolution
  #endif

  outgoingStream.println(String(axisDriversCount) + " steppers enabled");
	engine.init();

	for (int i = 0; i < axisDriversCount; i++) {
		axisDrivers[i]->begin(&engine, axisSettings[i]);
	}
}

void loop() {
  #if INCLUDE_WIFI
    ECrowneWifi::loop();
  #endif

  // Wait for data
  if (StreamAvailable() > 0) {
    String command = StreamReadStringUntil('\n');
    processCommand(command);
  }

  for (int i = 0; i < axisDriversCount; i++) {
    axisDrivers[i]->loop();
  }
}
