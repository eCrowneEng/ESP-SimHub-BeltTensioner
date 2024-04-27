#include <Arduino.h>
#include <EspSimHub.h>

// No longer have to define whether it's an ESP32 or ESP8266, just do an initial compilation and
//  VSCode will pick  up the right environment from platformio.ini

#define INCLUDE_WIFI true
// Less secure if you plan to commit or share your files, but saves a bunch of memory. 
//  If you hardcode credentials the device will only work in your network
#define USE_HARDCODED_CREDENTIALS false

#if INCLUDE_WIFI
#if USE_HARDCODED_CREDENTIALS
#define WIFI_SSID "Wifi NAME"
#define WIFI_PASSWORD "WiFi Password"
#endif

#define BRIDGE_PORT 10001 // Perle TruePort uses port 10,001 for the first serial routed to the client
#define DEBUG_TCP_BRIDGE false // emits extra events to Serial that show network communication, set to false to save memory and make faster

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


// insert here rest of code 


void setup()
{
#if INCLUDE_WIFI
  Serial.begin(115200);
#else
  Serial.begin(19200);
#endif
// StreamPrintln(String(axisDriversCount) + " steppers enabled");

#if INCLUDE_WIFI
  ECrowneWifi::setup(&outgoingStream, &incomingStream);
#endif


  // insert here setup code, replacing Serial.println with outgoingStream.println
}


void loop() {
#if INCLUDE_WIFI
  ECrowneWifi::loop();
#endif

  // Wait for data
  if (StreamAvailable() > 0) {
    String command = StreamReadStringUntil('\n');
    //processCommand(command);
  }

  // for (int i = 0; i < axisDriversCount; i++) {
  //   axisDrivers[i]->loop();
  // }
}
