#pragma once
#include "SHARED_DEFINES.h"
#include <WinSock2.h>
#include <unordered_map>	
#include <SharedStuff.h>

void SendBytes(SOCKET aSocket, char *aBuffer, int len);
BOOL WINAPI OnClose();
static SOCKET ourExitSocket = INVALID_SOCKET;
static char ourID = -1;

class Client
{
public:
	Client(char argc, char **argv);
	void Listen();
	void WriteToFile();
private:
	char myID = -1;
	char myWriteBuffer[DEFAULT_BUFLEN];
	SOCKET myServerSocket = INVALID_SOCKET;
	std::string myChatLog;
	std::string myName;
	std::unordered_map<char, std::string> myUsers;

	RecieveFile myGetFile;
	SendFile mySendFile;
};

