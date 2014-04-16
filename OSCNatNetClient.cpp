/*

OSCNatNetClient.cpp

This program connects to a NatNet server, receives a data stream, and writes that data stream
to an ascii file.  The purpose is to illustrate using the NatNetClient class.

Usage:

OSCNatNetClient [ServerIP] [LocalIP] [OSCIP] [OSCPort] <log>

[ServerIP]			IP address of the server (e.g. 192.168.0.107) ( defaults to local machine)
[OutputFilename]	Name of points file (pts) to write out.  defaults to Client-output.pts

*/

#include <stdio.h>
#ifdef __linux__
# include <cstring>
# include <cstdlib>
# include <time.h>

// good idea to use curses here?
#include <curses.h>
int _getch(void) {
  return getch();
}


#define __cdecl
typedef char _TCHAR;
#else
#include <tchar.h>
#include <conio.h>
#include <winsock2.h>
#include <time.h>
#include <windows.h>
#endif

#include "NatNetTypes.h"
#include "NatNetClient.h"

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#ifdef HAVE_GITVERSION_H
# include "gitversion.h"
#endif


#ifndef MAX_PATH
# define MAX_PATH 1024
#endif

#ifdef _MSC_VER
# pragma warning( disable : 4996 )
#endif


void _WriteHeader(FILE* fp, sDataDescriptions* pBodyDefs);
void _WriteFrame(FILE* fp, sFrameOfMocapData* data);
void _WriteFooter(FILE* fp);
void __cdecl DataHandler(sFrameOfMocapData* data, void* pUserData);			// receives data from the server
void __cdecl MessageHandler(int msgType, char* msg);		// receives NatNet error mesages
void resetClient();
char* _getOSCTimeStamp(char[100]);

NatNetClient theClient;
FILE* fps = NULL;
FILE* fpf = NULL;

char szMyIPAddress[128] = "";
char szServerIPAddress[128] = "";


UdpTransmitSocket* transmitSocket;
char* OSCIP = NULL;
int OSCPort = 0;

void splash(_TCHAR*name) {
	printf("%s: NatNet to OSC bridge\n", name);
#ifdef GITREVISION
	printf("\trev:%s compiled on %s\n", GITREVISION, __DATE__);
#else
	printf("\tcompiled on %s\n", __DATE__);
#endif
}


int _tmain(int argc, _TCHAR* argv[])
{
  splash(argv[0]);

  if((argc < 4) || ((argc > 5) && (strcmp("log", argv[5])))){
    printf("Usage: %s <ServerIP> <ClientIP> <OSCIP> <OSCPort> [\"log\"]", argv[0]);
  }else{


    int retCode;
    char* OSCIP = argv[3];
    int OSCPort = atoi(argv[4]);

    //EU
    printf("\n\nBanana hammock!!\n\n");

    // Set callback handlers
    theClient.SetMessageCallback(MessageHandler);
    theClient.SetVerbosityLevel(Verbosity_Debug);
    theClient.SetDataCallback( DataHandler, &theClient );	// this function will receive data from the server

    // Connect to NatNet server
    strcpy(szServerIPAddress, argv[1]);	// specified on command line
    strcpy(szMyIPAddress, argv[2]);	// specified on command line
    printf("Connecting to server at %s from %s...\n", szServerIPAddress, szMyIPAddress);

    // Connect to NatNet server


    retCode = theClient.Initialize(szMyIPAddress, szServerIPAddress);
    if (retCode != ErrorCode_OK)
      {
	printf("Unable to connect to server.  Error code: %d. Exiting", retCode);
	return 1;
      }
    else
      {

	// print server info
	sServerDescription ServerDescription;
	memset(&ServerDescription, 0, sizeof(ServerDescription));
	theClient.GetServerDescription(&ServerDescription);
	if(!ServerDescription.HostPresent)
	  {
	    printf("Unable to connect to server. Host not present. Exiting.");
	    return 1;
	  }
	printf("[OSCNatNetClient] Server application info:\n");
	printf("Application: %s (ver. %d.%d.%d.%d)\n",
	       ServerDescription.szHostApp,
	       ServerDescription.HostAppVersion[0],
	       ServerDescription.HostAppVersion[1],
	       ServerDescription.HostAppVersion[2],
	       ServerDescription.HostAppVersion[3]);
	printf("NatNet Version: %d.%d.%d.%d\n",
	       ServerDescription.NatNetVersion[0],
	       ServerDescription.NatNetVersion[1],
	       ServerDescription.NatNetVersion[2],
	       ServerDescription.NatNetVersion[3]);
	printf("Server IP:%s\n", szServerIPAddress);
	printf("Server Name:%s\n\n", ServerDescription.szHostComputerName);

	//EU - init UDP socket
	transmitSocket = new UdpTransmitSocket(IpEndpointName(OSCIP, OSCPort));
	printf("OSC IP:%s\n", OSCIP);
	printf("OSC Port:%d\n", OSCPort);
      }

    // send/receive test request
    printf("[OSCNatNetClient] Sending Test Request\n");
    void* response;
    int nBytes;
    retCode = theClient.SendMessageAndWait("TestRequest", &response, &nBytes);
    if (retCode == ErrorCode_OK)
      {
	printf("[OSCNatNetClient] Received: %s", (char*)response);
      }




    //Writing Session data to file

    char szFile[MAX_PATH];
    char szFolder[MAX_PATH];
#ifdef __linux__
    sprintf(szFolder, "./");
#else
    GetCurrentDirectory(MAX_PATH, szFolder);
#endif
    char timeLabel[50];
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    //create time label for files
    strftime (timeLabel,80,"%d.%m.%y %H.%M.%S",timeinfo);

    // Retrieve MarkerSets from server
    printf("\n\n[OSCNatNetClient] Requesting all data definitions for current session...");
    sDataDescriptions* pDataDefs = NULL;
    theClient.GetDataDescriptions(&pDataDefs);
    if(!pDataDefs){

      printf("[OSCNatNetClient] Unable to retrieve current session.");
      //return 1;

    }else{
      sprintf(szFile, "%s\\SessionData %s.txt", szFolder, timeLabel);
      fps = fopen(szFile, "w");
      if(!fps)
	{
	  printf("error opening output file %s.  Exiting.", szFile);
	  exit(1);
	}
      _WriteHeader(fps, pDataDefs);
      fclose(fps);
    }


    // Prepare data file for frame info

    if(argc > 5){

      sprintf(szFile, "%s\\FrameData %s.txt", szFolder, timeLabel);
      fpf = fopen(szFile, "w");
      if(!fpf){
	printf("error opening output file %s.  Exiting.", szFile);
	exit(1);
      }
    }



    // Ready to receive marker stream!
    printf("\nOSCNatNetClient is connected to server and listening for data... press 'q' for quitting\n");
    int c;
    bool bExit = false;
    while(c =_getch()){
      switch(c){
      case 'q':
	bExit = true;
	printf("\nQuitting...\n\n");
	break;
      case 'r':
	printf("\nReseting Client...\n\n");
	resetClient();
	break;
      default:
	printf("not an option\n\n");
	break;
      }
      if(bExit)
	break;
    }

    printf("clean up\n\n");
    // Done - clean up.
    theClient.Uninitialize();
    if(fpf != NULL) fclose(fpf);

    return ErrorCode_OK;
  }
}




//*****************************************************
  //*****************************************************
    //*****************************************************



      // DataHandler receives data from the server
    void __cdecl DataHandler(sFrameOfMocapData* data, void* pUserData)
{

  NatNetClient* pClient = (NatNetClient*) pUserData;

  printf("Received data frame %d\n", data->iFrame);
  if(fpf) _WriteFrame(fpf,data);


  int i,j;
  char ts[100];
  char buffer[(sizeof(sFrameOfMocapData))];
  osc::OutboundPacketStream p(buffer, sizeof(sFrameOfMocapData));

  p << osc::BeginBundle(getOscTime());

#if 0
  // timestamp
  p << osc::BeginMessage("/ts");
  p << _getOSCTimeStamp(ts);
  p << osc::EndMessage;
#endif

  //srv latency
  p << osc::BeginMessage("/srvlag");
  p << data->fLatency;
  p << osc::EndMessage;


  // Mocap MarkerSet Markers
  for(j=0; j < data->nMarkerSets; j++){
    for(i =0; i < data->MocapData[j].nMarkers; i++){
      // ATENCAO AO SPRINTF
	  char ns[300];
      sprintf(ns,"/markerset/%s/%d", data->MocapData[j].szName, i);
      p << osc::BeginMessage(ns);
      p << data->MocapData[j].Markers[i][0];
      p << data->MocapData[j].Markers[i][1];
      p << data->MocapData[j].Markers[i][2];
      p << osc::EndMessage;
    }
  }

  // Other Markers
  for(i=0; i < data->nOtherMarkers; i++){
    char ns[50];
	sprintf(ns,"/othermarker/%d", i);
    p << osc::BeginMessage(ns);
    p << data->OtherMarkers[i][0];
    p << data->OtherMarkers[i][1];
    p << data->OtherMarkers[i][2];
    p << osc::EndMessage;
  }


  // Rigid Bodies
  for(i=0; i < data->nRigidBodies; i++){
	char ns[50];
    sprintf(ns,"/rigidbody/%d", data->RigidBodies[i].ID);
    p << osc::BeginMessage(ns);
    p << data->RigidBodies[i].x;
    p << data->RigidBodies[i].y;
    p << data->RigidBodies[i].z;
    p << data->RigidBodies[i].qx;
    p << data->RigidBodies[i].qy;
    p << data->RigidBodies[i].qz;
    p << data->RigidBodies[i].qw;
    p << osc::EndMessage;

    for(int iMarker=0; iMarker < data->RigidBodies[i].nMarkers; iMarker++){
      char nsMarker[50];
      sprintf(nsMarker,"/rigidbody/%d/%d", data->RigidBodies[i].ID, iMarker);
      p << osc::BeginMessage(nsMarker);
      p << data->RigidBodies[i].Markers[iMarker][0];
      p << data->RigidBodies[i].Markers[iMarker][1];
      p << data->RigidBodies[i].Markers[iMarker][2];
      p << osc::EndMessage;
    }
  }
  p << osc::EndBundle;

  if(transmitSocket)transmitSocket->Send(p.Data(), p.Size());
}

// _WriteFrame writes data from the server to log file
void _WriteFrame(FILE* fp, sFrameOfMocapData* data)
{
  int i, j;

  if(fp == NULL)
    printf("file null in write frame\n\n");
  else{
    fprintf(fp, "Frame %d\n\n", data->iFrame);

    // Mocap MarkerSet Markers
    fprintf(fp,"Mocap MarkerSets [Count=%d]\n", data->nMarkerSets);

    for(j=0; j < data->nMarkerSets; j++){
      fprintf(fp,"MarkerSet %d name: %s\n", j, data->MocapData[j].szName);

      for(i =0; i < data->MocapData[j].nMarkers; i++){
	fprintf(fp, "Marker %d - x:%.5f y:%.5f z:%.5f\n",
		i,
		data->MocapData[j].Markers[i][0],
		data->MocapData[j].Markers[i][1],
		data->MocapData[j].Markers[i][2]);
	fprintf(fp, "\n");
      }
    }

    // Other Markers
    fprintf(fp,"Other Markers [Markers Count=%d]\n", data->nOtherMarkers);
    for(i=0; i < data->nOtherMarkers; i++)
      fprintf(fp, "Marker %d - x:%3.2f y:%3.2f z:%3.2f\n",
	      i,
	      data->OtherMarkers[i][0],
	      data->OtherMarkers[i][1],
	      data->OtherMarkers[i][2]);
    fprintf(fp, "\n");

    // Rigid Bodies
    fprintf(fp, "Rigid Bodies [Count=%d]\n", data->nRigidBodies);
    for(i=0; i < data->nRigidBodies; i++){
      fprintf(fp,"Rigid Body ID=%d\n", data->RigidBodies[i].ID);
      fprintf(fp,"x:%3.2f y:%3.2f z:%3.2f qx:%3.2f qy:%3.2f qz:%3.2f qw:%3.2f\n",
	      data->RigidBodies[i].x,
	      data->RigidBodies[i].y,
	      data->RigidBodies[i].z,
	      data->RigidBodies[i].qx,
	      data->RigidBodies[i].qy,
	      data->RigidBodies[i].qz,
	      data->RigidBodies[i].qw);

      fprintf(fp, "\n");
      fprintf(fp,"Rigid body markers [Markers Count=%d]\n", data->RigidBodies[i].nMarkers);
      for(int iMarker=0; iMarker < data->RigidBodies[i].nMarkers; iMarker++)
	fprintf(fp,"Marker %d - x:%3.2f y:%3.2f z:%3.2f\n",
		i,
		data->RigidBodies[i].Markers[iMarker][0],
		data->RigidBodies[i].Markers[iMarker][1],
		data->RigidBodies[i].Markers[iMarker][2]);
    }
    fprintf(fp, "\n\n\n");
  }
}



/* File writing routines */
void _WriteHeader(FILE* fp, sDataDescriptions* pDataDefs)
{
  sMarkerSetDescription* pMS;
  sRigidBodyDescription* pRB;

  for(int j=0; j < pDataDefs->nDataDescriptions; j++){
    fprintf(fp,"[OSCNatNetClient] Received Data Descriptions:\n");

    //DataDescriptors type;
    //type = Descriptor_MarkerSet;
    switch(pDataDefs->arrDataDescriptions[j].type){
    case (0):
      pMS = pDataDefs->arrDataDescriptions[j].Data.MarkerSetDescription;
      fprintf(fp, "[OSCNatNetClient] MarkerSet: %s\n", pMS->szName);
      for(int i=0; i < pMS->nMarkers; i++)
	fprintf(fp, "[OSCNatNetClient]Marker %i: %s\n", i, pMS->szMarkerNames[i]);
      break;
    case 1:
      pRB = pDataDefs->arrDataDescriptions[j].Data.RigidBodyDescription;
      fprintf(fp, "[OSCNatNetClient] Rigid body ID: %d\n", pRB->ID);
      fprintf(fp, "[OSCNatNetClient] Rigid body parent ID: %d\n", pRB->parentID);
      fprintf(fp, "[OSCNatNetClient] Rigid body offset: %f %f %f\n", pRB->offsetx, pRB->offsety, pRB->offsetz);
      break;
    case 2:
      fprintf(fp, "[OSCNatNetClient] Skeleton... no data!\n");
    default:
      fprintf(fp, "[OSCNatNetClient] Non identified data desc...\n");
      break;
    }
  }
}

// MessageHandler receives NatNet error/debug messages
void __cdecl MessageHandler(int msgType, char* msg)
{
  printf("\n%s\n", msg);
}


void resetClient()
{
  int iSuccess;

  printf("\n\nre-setting Client\n\n.");

  iSuccess = theClient.Uninitialize();
  if(iSuccess != 0)
    printf("error un-initting Client\n");

  iSuccess = theClient.Initialize(szMyIPAddress, szServerIPAddress);
  if(iSuccess != 0)
    printf("error re-initting Client\n");


}

char* _getOSCTimeStamp(char ts[100]){
  unsigned int tmili = 0;
#ifdef _WIN32
  SYSTEMTIME lt;
  GetLocalTime(&lt);
  tmili = ((int)lt.wHour)*3600000 + ((int)lt.wMinute)*60000 + ((int)lt.wSecond)*1000 + ((int)lt.wMilliseconds);
#endif
  sprintf(ts, "%d", tmili);

  return ts;
}
#define SECONDS_FROM_1900_to_1970 2208988800UL /* 17 leap years */
#define TWO_TO_THE_32_OVER_ONE_MILLION 4295
osc::uint64 getOscTime(void) {
  /* from http://www.cnmat.berkeley.edu/OpenSoundControl/OSC-spec.html#timetags
     Time tags are represented by a 64 bit fixed point number.
     The first 32 bits specify the number of seconds since midnight on January 1, 1900,
     and the last 32 bits specify fractional parts of a second to a precision of about 200 picoseconds.
     This is the representation used by Internet NTP timestamps.
     The time tag value consisting of 63 zero bits followed by a one in the least signifigant bit
     is a special case meaning "immediately."
  */
  osc::uint64 result, secOffset, usecOffset;
  long unsigned sec, usec;

  // this is specific for gcc: ULL at the end to fit into long, remove this for msvc
#ifdef __WIN32__
  long unsigned secBetween1601and1970 = 11644473600ULL;
  FILETIME fileTime;
  GetSystemTimeAsFileTime(&fileTime);
  sec  = (* (unsigned __int64 *) &fileTime / (unsigned __int64)10000000) - secBetween1601and1970;
  usec = (* (unsigned __int64 *) &fileTime % (unsigned __int64)10000000)/(unsigned __int64)10;
#else
  timeval tv;
  gettimeofday(&tv, 0);
  sec = tv.tv_sec;
  usec= tv.tv_usec;
#endif

  /* First get the seconds right */
  secOffset = (unsigned) SECONDS_FROM_1900_to_1970 + (unsigned) sec;
  /* Now get the fractional part. */
  usecOffset = (unsigned) usec * (unsigned) TWO_TO_THE_32_OVER_ONE_MILLION;

  /* make seconds the high-order 32 bits */
  result = secOffset << 32;
  result += usecOffset;

  return result;
}
