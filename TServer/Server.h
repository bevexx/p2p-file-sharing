#pragma once
#include <unordered_map>
#include <WinSock2.h>
#include <SharedStuff.h>

class Server
{
public:
	Server(char argc, char **argv);

	void WriteToFile();
	void EchoToOtherUsers(const RecieveFile& aFile);
private:
	SOCKET myClientSocket = INVALID_SOCKET;
	std::unordered_map<char, std::string> myUsers;
	std::vector<SOCKADDR_IN> myAdresses;

	RecieveFile myFile;
	
};

