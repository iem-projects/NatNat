all: NatNat.exe

SDK_PATH = C:\Programme\Microsoft SDKs\Windows\v6.0A\Lib
VC_PATH = $(VCINSTALLDIR)\lib
WIN_LIB = /NODEFAULTLIB:libcmt /NODEFAULTLIB:msvcrt /NODEFAULTLIB:oldnames /NODEFAULTLIB:kernel32 "$(SDK_PATH)"\Kernel32.lib "$(VC_PATH)"\oldnames.lib "$(VC_PATH)"\msvcrt.lib ws2_32.lib winmm.lib

VPATH=Debug:.

OBJ=OSCNatNetClient.obj OscOutboundPacketStream.obj OscPrintReceivedElements.obj OscReceivedElements.obj OscTypes.obj UdpSocket.obj IpEndpointName.obj NetworkingUtils.obj

LDFLAGS=/nologo "C:\\$(HOMEPATH)"\Desktop\NaturalTrack\NatNetSDK\lib\NatNetLibStatic.lib $(WIN_LIB)


NatNat.exe: $(OBJ)
	link /out:NatNat.exe $(OBJ) $(LDFLAGS)