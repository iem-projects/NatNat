default: OSCNatNetClient

VPATH = .:osc:ip

CXXFLAGS += -INatNetSDK/include

OSCSRC=OscTypes.cpp \
	OscOutboundPacketStream.cpp \
	OscReceivedElements.cpp \
	OscPrintReceivedElements.cpp

NETSRC=UdpSocket.cpp \
	NetworkingUtils.cpp \
	IpEndpointName.cpp

SRC=OSCNatNetClient.cpp \
	$(OSCSRC) \
	$(NETSRC)

OBJ=$(SRC:%.cpp=%.o)

%.cpp.o: %.o

OSCNatNetClient: $(OBJ)
	$(CXX) -o $@ $^

