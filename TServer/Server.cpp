#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Server.h"
#include <iostream>
#include <ws2tcpip.h>
#include <stdio.h>
#include "SHARED_DEFINES.h"
#include "commctrl.h"
#include <thread>
#include "Messages/CNetMessageChatMessage.h"
#include <Messages/CNetMessageFile.h>
#pragma comment(lib, "Ws2_32.lib")

#define min(a, b) (a < b ? a : b)

Server::Server(char argc, char **argv)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 10);

	WSADATA wsaData;

	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %i\n", iResult);
		return;
	}

	// Set adress info
	struct addrinfo *result = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %i\n", iResult);
		WSACleanup();
		return;
	}

	// Create socket
	myClientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (myClientSocket == INVALID_SOCKET)
	{
		printf("error at socket():  %i\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	printf("Enter the ip you want to connect to: ");
	std::string ip = DEFAULT_SERVER_IP_INTERNAL;
	if (std::cin.peek() != '\n')
		std::cin >> ip;
	std::cin.ignore();

	SOCKADDR_IN receiverAddr;
	receiverAddr.sin_family = AF_INET;
	receiverAddr.sin_port = htons(DEFAULT_PORT_NUMBER);
	receiverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	// Bind socket
	iResult = bind(myClientSocket, (SOCKADDR *)&receiverAddr, sizeof(receiverAddr));
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %i\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(myClientSocket);
		WSACleanup();
		return;
	}

	std::cout << "Server started on " << ip << "\n";

	freeaddrinfo(result);

	// Set up time interval
	timeval timeout;
	timeout.tv_sec = 1000;
	timeout.tv_usec = 0;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(myClientSocket, &fds);
	int selectTiming = select(0, &fds, 0, 0, &timeout);

	char receiveBuf[DEFAULT_BUFLEN];
	int bytesReceived = 0;
	SOCKADDR_IN senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	char IDmaker = 0;

	std::cout << "Listening... " << "\n";
	switch (selectTiming)
	{
	case 0:
		printf("Timeout lor while waiting... -.-\n");
		break;
	case -1:
		printf("Error: %ld\n", WSAGetLastError());
		break;
	default:
		while (1)
		{
			// Call recvfrom() to get it then display the received data...
			memset(receiveBuf, '\0', DEFAULT_BUFLEN);
			bytesReceived = recvfrom(myClientSocket, receiveBuf, DEFAULT_BUFLEN, 0, (SOCKADDR *)&senderAddr, &senderAddrSize);
			if (bytesReceived > 0)
			{

				char *dataFromNet = receiveBuf;
				char netBufIn[DEFAULT_BUFLEN];
				memcpy(&netBufIn, dataFromNet, bytesReceived);

				if ((ClassTypeID)netBufIn[0] == ClassTypeID::StringMessage)
				{
					CNetMessageChatMessage newMessage;
					memcpy(&newMessage, &netBufIn, bytesReceived);

					std::string data(newMessage.GetText());
					if (data.substr(0, 10) == "handshake_")
					{
						std::string trimName = data.substr(10, data.size() - 10);
						std::cout << trimName << " connected to the Server" << "\n";
						myAdresses.push_back(senderAddr);

						CNetMessageChatMessage outputMessage;
						outputMessage.myID = IDmaker;

						std::string message = "users_list.";
						for (auto &value : myUsers)
						{
							message += std::to_string((int)value.first) + "_" + value.second + ".";
						}

						message += "\nh_" + trimName;
						myUsers.emplace(IDmaker++, trimName);
						outputMessage.SetMessage(message);
						memcpy(netBufIn, &outputMessage, sizeof(CNetMessageChatMessage));

						for (auto &addr : myAdresses)
						{
							int iSendResult = sendto(myClientSocket, netBufIn, CHAT_BUFLEN, 0, (SOCKADDR *)&addr, senderAddrSize);
							if (iSendResult == SOCKET_ERROR)
							{
								printf("send failed: %d\n", WSAGetLastError());
								closesocket(myClientSocket);
								WSACleanup();
								return;
							}
						}
					}
					else if (data == "server_leave")
					{
						std::cout << myUsers.at(newMessage.myID) << " has disconnected" << "\n";
						for (auto &addr : myAdresses)
						{
							int iSendResult = sendto(myClientSocket, netBufIn, CHAT_BUFLEN, 0, (SOCKADDR *)&addr, senderAddrSize);
							if (iSendResult == SOCKET_ERROR)
							{
								printf("send failed: %d\n", WSAGetLastError());
								closesocket(myClientSocket);
								WSACleanup();
								return;
							}
						}
					}
					else if (data.find("1337_writingfile_") != std::string::npos)
					{
						int pos = data.find("1337_writingfile_");
						myFileName = data.substr(17, data.size() - 17);
						myFile.open(myFileName, std::fstream::out | std::fstream::binary);

						if (!myFile.is_open())
						{
							std::cout << "server ignored file write";
							continue;
						}

						myTotalFileSize = newMessage.mySize;
						myFile.flush();
						myFileBuf = myFile.rdbuf();

						int iSendResult = SOCKET_ERROR;
						while (iSendResult == SOCKET_ERROR)
						{
							iSendResult = sendto(myClientSocket, netBufIn, CHAT_BUFLEN, 0, (SOCKADDR *)&senderAddr, senderAddrSize);
						}
					}
					else if (data == "1337_finished")
					{
						WriteToFile();
					}
					else
					{
						std::cout << myUsers.at(newMessage.myID) << ": " + data << "\n";
						for (auto &addr : myAdresses)
						{
							int iSendResult = sendto(myClientSocket, netBufIn, CHAT_BUFLEN, 0, (SOCKADDR *)&addr, senderAddrSize);
							if (iSendResult == SOCKET_ERROR)
							{
								printf("send failed: %d\n", WSAGetLastError());
								closesocket(myClientSocket);
								WSACleanup();
								return;
							}
						}
					}
				}
				else if ((ClassTypeID)netBufIn[0] == ClassTypeID::File)
				{
					if (!myFile.is_open())
					{
						continue;
					}

					CNetMessageFile newMessage;
					memcpy(&newMessage, &netBufIn, bytesReceived);

					if (myFilePacketsGot > newMessage.myPosition)
					{
						continue;
					}

					CNetMessageChatMessage callback;
					callback.SetMessage("1337_confirm");
					callback.myPosition = myFilePacketsGot++;
					callback.mySize = newMessage.mySize;

					std::cout << "retrieving " << myFileName << ": (" << (float(min(FILE_BUFLEN * myFilePacketsGot, myTotalFileSize)) / myTotalFileSize) * 100.0f << "%) "
						<< min(FILE_BUFLEN * myFilePacketsGot, myTotalFileSize) / 1000 << "kb / " << myTotalFileSize / 1000 << "kb             \r";

					myFile.write(newMessage.myMessage, newMessage.mySize);
					if (newMessage.mySize < FILE_BUFLEN)
					{
						CNetMessageChatMessage callback;
						callback.SetMessage("1337_done");
						char sendBuffer[CHAT_BUFLEN];
						memcpy(sendBuffer, &callback, sizeof(callback));
						sendto(myClientSocket, sendBuffer, CHAT_BUFLEN, 0, (SOCKADDR *)&senderAddr, senderAddrSize);

						WriteToFile();
						continue;
					}
					
					char sendBuffer[CHAT_BUFLEN];
					memcpy(sendBuffer, &callback, sizeof(callback));

					int iSendResult = SOCKET_ERROR;
					while (iSendResult == SOCKET_ERROR)
					{
						iSendResult = sendto(myClientSocket, sendBuffer, CHAT_BUFLEN, 0, (SOCKADDR *)&senderAddr, senderAddrSize);
					}
				}
				

			}
			else if (bytesReceived <= 0)
				printf("Server: Connection closed with error code : % ld\n", WSAGetLastError());
			else
				printf("Server: recvfrom() failed with error code : % d\n", WSAGetLastError());

		}
	}
}

void Server::WriteToFile()
{
	myFile.close();						
	std::cout << "retrieved " << myFileName << "!\n";
}