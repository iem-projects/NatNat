default: OSCNatNetClient

VPATH = .:osc:ip

NATNETSDK=/usr/local/NatNetSDK

NATNETINCLUDEDIR=$(NATNETSDK)/include
NATNETLIBDIR=$(NATNETSDK)/bin

CXXFLAGS+=-I$(NATNETINCLUDEDIR)
LDFLAGS+=-L$(NATNETLIBDIR)

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

