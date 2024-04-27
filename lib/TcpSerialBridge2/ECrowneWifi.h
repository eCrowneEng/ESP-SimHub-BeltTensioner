#include <TcpSerialBridge2.h>
#include <FullLoopbackStream.h>

TcpSerialBridge2 instance(BRIDGE_PORT);

class ECrowneWifi {
    public:
        static void setup(FullLoopbackStream *outgoingStream, FullLoopbackStream *incomingStream) {
            instance.setup(outgoingStream, incomingStream);
        }

        static void loop() {
            instance.loop();
        }

        static void flush() {
            instance.flush();
        }
};
