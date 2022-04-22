#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Client.h"
#include <iostream>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")

#include "Messages/CNetMessageChatMessage.h"
#include "Messages/CNetMessageFile.h"

Client::Client(char argc, char **argv)
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)OnClose, TRUE);

	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 12);

	WSADATA wsaData;

	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup failed: " << iResult << "\n";
		return;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %i\n", iResult);
		WSACleanup();
		return;
	}
	ptr = result;

	// Create socket
	myServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (myServerSocket == INVALID_SOCKET)
	{
		printf("error at socket(): %i\n", WSAGetLastError());
		//freeaddrinfo(result);
		WSACleanup();
		return;
	}

	printf("Enter the ip you want to connect to: ");
	std::string ip = DEFAULT_SEVER_IP_EXTERNAL;
	if (std::cin.peek() != '\n')
		std::cin >> ip;
	std::cin.ignore();

	SOCKADDR_IN receiverAddr;
	receiverAddr.sin_family = AF_INET;
	receiverAddr.sin_port = htons(DEFAULT_PORT_NUMBER);
	receiverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	// Connect to server
	iResult = connect(myServerSocket, (SOCKADDR *)&receiverAddr, sizeof(receiverAddr));
	//iResult = connect(myServerSocket, ptr->ai_addr, ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		closesocket(myServerSocket);
		myServerSocket = INVALID_SOCKET;
	}

	// *Testa nästa socket osv..*


	//u_long ne = TRUE;
	//iResult = ioctlsocket(myServerSocket, FIONBIO, &ne);
	//if (iResult == SOCKET_ERROR)
	//{
	//	printf("Failed to make non-blocking...\n");
	//	WSACleanup();
	//	return;
	//}

	freeaddrinfo(result);

	if (myServerSocket == INVALID_SOCKET)
	{
		printf("unable to connect to server...\n");
		WSACleanup();
		return;
	}

	ourExitSocket = myServerSocket;

	std::thread listenThread(&Client::Listen, this);
	listenThread.detach();

	printf("Skriv ditt namn: ");
	std::cin >> myName;
	std::string prefixNamn = "handshake_" + myName;

	CNetMessageChatMessage message;
	message.SetMessage(prefixNamn);
	memcpy(myWriteBuffer, &message, sizeof(CNetMessageChatMessage));
	SendBytes(myServerSocket, myWriteBuffer, CHAT_BUFLEN);
	system("CLS");

	std::string writeString = "";
	while (true)
	{
		SetConsoleTextAttribute(console, 10);
		printf(myChatLog.c_str());
		SetConsoleTextAttribute(console, 12);

		bool hasLooped = false;
		while (myFile.is_open())
		{
			std::cout << "sending " << writeString << ": (" << (float(min(FILE_BUFLEN * myPacketIndex, myTotalFileSize)) / myTotalFileSize) * 100.0f << "%) "
				<< min(FILE_BUFLEN * myPacketIndex, myTotalFileSize) / 1000 << "kb / " << myTotalFileSize / 1000 << "kb\r";

			hasLooped = true;
			//Sleep(500);
			//system("CLS");
			//continue;
		}
		if (hasLooped)
		{
			system("CLS");
			SetConsoleTextAttribute(console, 10);
			printf(myChatLog.c_str());
			SetConsoleTextAttribute(console, 12);
		}

		printf("Enter chat message: ");
		getline(std::cin, writeString);
		system("CLS");

		if (writeString != "" && !myFile.is_open())
		{
			if (writeString.find(".") != std::string::npos && writeString.find(".") != writeString.size() - 1 && writeString.find(".") != 0)
			{
				myFile.open(writeString, std::fstream::in | std::fstream::binary);

				if (!myFile.is_open() && myFile.gcount() <= 0)
				{
					myChatLog += "/*SERVER*/ File not found\n";
					continue;
				}
				myChatLog += "/*SERVER*/ Started sending " + writeString + "...\n";

				message.SetMessage("1337_writingfile_" + writeString);
				message.myID = myID;

				myTotalFileSize = myFile.rdbuf()->pubseekoff(0, myFile.end);
				myFile.rdbuf()->pubseekoff(0, myFile.beg);

				message.mySize = myTotalFileSize;
				memcpy(myWriteBuffer, &message, sizeof(CNetMessageChatMessage));
				SendBytes(myServerSocket, myWriteBuffer, CHAT_BUFLEN);
			}
			else
			{
				message.SetMessage(writeString);
				message.myID = myID;
				memcpy(myWriteBuffer, &message, sizeof(CNetMessageChatMessage));
				SendBytes(myServerSocket, myWriteBuffer, CHAT_BUFLEN);
			}

		}
		else
		{
			writeString = "";
		}
	}
}

void SendBytes(SOCKET aSocket, char* aBuffer, int len)
{
	int iResult = send(aSocket, aBuffer, len, 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed: %i\n", WSAGetLastError());
		closesocket(aSocket);
		WSACleanup();
	}
}

void Client::Listen()
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	int iResult = 0;
	char buffer[CHAT_BUFLEN];
	SOCKADDR_IN remoteAddr;
	int         remoteAddrLen = sizeof(SOCKADDR_IN);
	do
	{
		iResult = recvfrom(myServerSocket, buffer, CHAT_BUFLEN, 0, (SOCKADDR*)&remoteAddr, &remoteAddrLen);
		char *dataFromNet = buffer;
		char netBufIn[CHAT_BUFLEN];
		const int sizeOfBytesGot = CHAT_BUFLEN;
		memcpy(&netBufIn, dataFromNet, sizeOfBytesGot);

		if ((ClassTypeID)netBufIn[0] == ClassTypeID::StringMessage)
		{
			CNetMessageChatMessage newMessage;
			memcpy(&newMessage, &netBufIn, sizeof(CNetMessageChatMessage));

			std::string data(newMessage.GetText());
			if (data.substr(0, 10) == "users_list")
			{
				data = data.substr(data.find('.') + 1, data.size() - data.find('.'));
				while (data.find('.') != std::string::npos)
				{
					auto pos = data.find('.');
					std::string sub = data.substr(0, pos);

					std::string key = sub.substr(0, sub.find('_'));
					std::string value = sub.substr(sub.find('_') + 1, sub.size() - sub.find('-'));

					if (myUsers.find(key[0]) == myUsers.end())
					{
						int num;
						std::stringstream ss;
						ss << key;
						ss >> num;

						myUsers.emplace(num, value);
					}

					data = data.substr(pos + 1, data.size() - pos);
				}
				

				data = data.substr(data.find('\n') + 1, data.size() - data.find('\n'));
				std::string trimName = data.substr(2, data.size() - 2);
				myUsers.emplace(newMessage.myID, trimName);
				myChatLog += trimName + " connected to the Server\n";
				if (trimName == myName)
				{
					myID = newMessage.myID;
					ourID = myID;
				}

				system("CLS");
				SetConsoleTextAttribute(console, 10);
				printf(myChatLog.c_str());
				SetConsoleTextAttribute(console, 12);
				printf("Enter chat message: ");
			}
			else if (data == "server_leave")
			{
				myChatLog += myUsers.at(newMessage.myID) + " has disconnected\n";
				system("CLS");
				SetConsoleTextAttribute(console, 10);
				printf(myChatLog.c_str());
				SetConsoleTextAttribute(console, 12);
				printf("Enter chat message: ");
			}
			else if (data.find("1337_writingfile_") != std::string::npos)
			{
				if (!myFile.is_open())
				{
					continue;
				}

				CNetMessageFile newFileFragment;
				newFileFragment.myClassTypeID = ClassTypeID::File;

				int currentSize = min(myTotalFileSize - FILE_BUFLEN * myPacketIndex, FILE_BUFLEN);
				myFile.read(newFileFragment.myMessage, currentSize);
				
				newFileFragment.myPosition = myPacketIndex++;
				newFileFragment.mySize = currentSize;

				memcpy(myWriteBuffer, &newFileFragment, DEFAULT_BUFLEN);
				SendBytes(myServerSocket, myWriteBuffer, DEFAULT_BUFLEN);
			}
			else if (data == "1337_confirm")
			{
				if (!myFile.is_open())
				{
					continue;
				}
				
				CNetMessageFile newFileFragment;
				newFileFragment.myClassTypeID = ClassTypeID::File;

				int currentSize = min(myTotalFileSize - FILE_BUFLEN * myPacketIndex, FILE_BUFLEN);
				myFile.read(newFileFragment.myMessage, currentSize);

				newFileFragment.myPosition = myPacketIndex++;
				newFileFragment.mySize = currentSize;

				memcpy(myWriteBuffer, &newFileFragment, DEFAULT_BUFLEN);
				SendBytes(myServerSocket, myWriteBuffer, DEFAULT_BUFLEN);
			}
			else if (data == "1337_done")
			{
				myFile.close();
				myChatLog += "/*SERVER*/ File successfully sent!\n";
			}
			else
			{
				if (myUsers.find(newMessage.myID) != myUsers.end())
					myChatLog += myUsers.at(newMessage.myID) + ": " + newMessage.GetText() + "\n";
				
				system("CLS");
				SetConsoleTextAttribute(console, 10);
				printf(myChatLog.c_str());
				SetConsoleTextAttribute(console, 12);
				printf("Enter chat message: ");
			}
		}
		else if ((ClassTypeID)netBufIn[0] == ClassTypeID::File)
		{
			CNetMessageFile newMessage;
			memcpy(&newMessage, &netBufIn, sizeOfBytesGot);

		}

	} while (iResult > 0);
}

BOOL __stdcall OnClose()
{
	CNetMessageChatMessage message;
	message.SetMessage("server_leave");
	message.myID = ourID;
	char buffer[CHAT_BUFLEN];
	memcpy(buffer, &message, sizeof(CNetMessageChatMessage));
	SendBytes(ourExitSocket, buffer, CHAT_BUFLEN);
	return 0;
}