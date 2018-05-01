//BTN415 - MS3
//Ehsan Ekbatani, Sina Lahsaee, Cheng-Tuo Shueh

#ifndef MYSOCKET_H
#define MYSOCKET_H

#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <string>

//Enumeration of type SocketType that contains {CLIENT, SERVER}
enum SocketType {CLIENT, SERVER};

//Enumeration of type ConnectionType that contains {TCP, UDP}
enum ConnectionType {TCP, UDP};

//Constant integer that defines the DEFAULT_SIZE of the buffer space
const int DEFAULT_SIZE = 128;


class MySocket{
	//Refer to assignment specs for the purpose of each variable
	char * Buffer;
	SOCKET WelcomeSocket;
	SOCKET ConnectionSocket;
	struct sockaddr_in SvrAddr;
	struct sockaddr_in RespAddr;
	SocketType mySocket;
	std::string IPAddr;
	int Port;
	ConnectionType connectionType;
	bool bConnect = false;
	int MaxSize;
	WSADATA wsa_data;

public:
	//Constructor
	MySocket(SocketType, std::string, unsigned int, ConnectionType, unsigned int);
	//Destructor
	~MySocket();

	//Socket init functions
	bool StartWSA();

	bool ConnectTCP();
	bool DisconnectTCP();

	bool SetupUDP();
	bool TerminateUDP();

	//Data transfer functions
	
	int SendData(const char*, int);
	int GetData(char*);

	//Getters and Setters

	std::string GetIPAddr();
	bool SetIPAddr(std::string);

	int GetPort();
	bool SetPortNum(int);

	SocketType GetType();
	bool SetType(SocketType);
};

#endif

