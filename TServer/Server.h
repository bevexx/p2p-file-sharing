#pragma once
#include <WinSock2.h>
#include "SHARED_DEFINES.h"
#include <unordered_map>
#include <fstream>

class Server
{
public:
	Server(char argc, char **argv);
	void WriteToFile();
private:
	SOCKET myClientSocket = INVALID_SOCKET;
	std::unordered_map<char, std::string> myUsers;
	std::vector<SOCKADDR_IN> myAdresses;

	std::fstream myFile;
	std::filebuf *myFileBuf;
	std::string myFileName = "";
	int myTotalFileSize = 0;
	int myFilePacketsGot = 0;
};

