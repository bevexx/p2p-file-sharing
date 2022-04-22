#pragma once
#include "SHARED_DEFINES.h"
#include <WinSock2.h>
#include <fstream>
#include <unordered_map>	

void SendBytes(SOCKET aSocket, char *aBuffer, int len);
BOOL WINAPI OnClose();
static SOCKET ourExitSocket = INVALID_SOCKET;
static char ourID = -1;

class Client
{
public:
	Client(char argc, char **argv);
	void Listen();
private:
	char myID = -1;
	char myWriteBuffer[DEFAULT_BUFLEN];
	SOCKET myServerSocket = INVALID_SOCKET;
	std::string myChatLog;
	std::string myName;
	std::unordered_map<char, std::string> myUsers;

	std::fstream myFile;
	int myTotalFileSize = 0;
	int myPacketIndex = 0;
};

