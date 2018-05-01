//BTN415 - MS3
//Ehsan Ekbatani, Sina Lahsaee, Cheng-Tuo Shueh

#include <string>
#include "MySocket.h"

//Object constructor taking in socket type (TCP/UDP), IP, port, connection type (Server/Client), and buffer size
MySocket::MySocket(SocketType sock, std::string IP, unsigned int port, ConnectionType connection, unsigned int size) {
	//Initialize sockets to safe empty
	WelcomeSocket = INVALID_SOCKET;
	ConnectionSocket = INVALID_SOCKET;
	bConnect = false;
	
	//Assign properties
	SetType(sock);
	SetIPAddr(IP);
	SetPortNum(port);
	connectionType = connection;

	//Allocate size of buffer, if size is invalid, set to default
	if (size <= 0) {
		MaxSize = DEFAULT_SIZE;
	}
	else {
		MaxSize = size;
	}
	Buffer = new char[MaxSize];

	//Initialize the winsock DLL, proceed only if successful
	if (!StartWSA()) {
		std::cout << "Could not start DLLs" << std::endl;

		delete[] Buffer;
		Buffer = nullptr;
	}
}

//Object destructor
MySocket::~MySocket() {
	//Close all sockets
	closesocket(ConnectionSocket);
	ConnectionSocket = INVALID_SOCKET;
	closesocket(WelcomeSocket);
	WelcomeSocket = INVALID_SOCKET;

	WSACleanup();

	bConnect = false;

	//De-allocate dynamic memory
	delete[] Buffer;
	Buffer = nullptr;
}

/* SOCKET INIT FUNCTIONS */

//Start DLL and return result
bool MySocket:: StartWSA() {
	//WSAStartup() returns 0 if it's successful
	return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
}

//Set up TCP socket, return results
bool MySocket::ConnectTCP() {
	if (connectionType != TCP) {
		//Prevent non-TCP setups
		return false;
	}
	else {
		//This will be the welcome socket for TCP server, and the connection socket for TCP client (so we can init just once for both cases)
		WelcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		//Proceed only if successful
		if (WelcomeSocket == INVALID_SOCKET) {
			std::cout << "Could not initialize socket" << std::endl;

			delete[] Buffer;
			Buffer = nullptr;

			return false;
		}
		else {
			//Set the target address
			SvrAddr.sin_family = AF_INET;
			SvrAddr.sin_port = htons(Port);
			SvrAddr.sin_addr.s_addr = inet_addr(IPAddr.c_str());

			//Additional setup depending on connection type
			if (mySocket == SERVER) {
				//Bind TCP socket, proceeding only if successful
				if ((bind(this->WelcomeSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
					//Close welcome socket upon failure
					closesocket(this->WelcomeSocket);
					WelcomeSocket = INVALID_SOCKET;

					std::cout << "Could not bind to the TCP socket" << std::endl;

					delete[] Buffer;
					Buffer = nullptr;

					return false;
				}
				else {
					//Set TCP socket to listen mode, proceed only if successful
					if (listen(WelcomeSocket, 1) == SOCKET_ERROR) {
						closesocket(WelcomeSocket);
						WelcomeSocket = INVALID_SOCKET;

						std::cout << "Could not listen to the provided TCP socket." << std::endl;

						delete[] Buffer;
						Buffer = nullptr;

						return false;
					}
					else {
						//Accept incoming connections
						std::cout << "Waiting for TCP client connection" << std::endl;

						if ((ConnectionSocket = accept(this->WelcomeSocket, NULL, NULL)) == SOCKET_ERROR) {
							closesocket(this->WelcomeSocket);
							WelcomeSocket = INVALID_SOCKET;
							ConnectionSocket = INVALID_SOCKET;

							std::cout << "Could not accept incoming TCP connection." << std::endl;

							delete[] Buffer;
							Buffer = nullptr;

							return false;
						}
						else {
							//Return status of connection attempt
							bConnect = true;
							return bConnect;
						}
					}
				}
			}
			else if (mySocket == CLIENT) {
				//Attempt to connect to server and return status
				if ((connect(this->WelcomeSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
					closesocket(this->WelcomeSocket);
					WelcomeSocket = INVALID_SOCKET;

					std::cout <<  "Could not connect to the TCP server" << std::endl;

					return false;
				}
				else {
					bConnect = true;
					return bConnect;
				}
			}
			else {
				return false;
			}
		}
	}
}

//Attempt to disconnect the client-server connection
bool MySocket::DisconnectTCP() {
	if (connectionType != TCP) {
		//Prevent non-TCP setups
		return false;
	}
	else {
		//Close all sockets
		bool succ = true;
			
		if (mySocket == SERVER) {
			succ = closesocket(ConnectionSocket) != SOCKET_ERROR;
		}
		succ = closesocket(WelcomeSocket) != SOCKET_ERROR;

		if (succ) {
			ConnectionSocket = INVALID_SOCKET;
			WelcomeSocket = INVALID_SOCKET;
			bConnect = false;
		}

		return succ;
	}
}

//Set up UDP socket, return results
bool MySocket::SetupUDP() {
	if (connectionType != UDP) {
		//Prevent non-UDP setups
		return false;
	}
	else {
		ConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		//Proceed only if successful
		if (ConnectionSocket == INVALID_SOCKET) {
			std::cout << "Could not initialize socket" << std::endl;

			delete[] Buffer;
			Buffer = nullptr;

			return false;
		}
		else {
			if (mySocket == SERVER) {
				//Allow any address
				SvrAddr.sin_family = AF_INET;
				SvrAddr.sin_port = htons(Port);
				SvrAddr.sin_addr.s_addr = INADDR_ANY;

				//Bind UDP server
				if (bind(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
					closesocket(ConnectionSocket);
;
					std::cout << "Could not bind to the UDP socket" << std::endl;

					return false;
				}
				else {
					std::cout << "Waiting for UDP datagrams" << std::endl;
					bConnect = true;

					return bConnect;
				}
			}
			else if (mySocket == CLIENT) {
				//No further action required for UDP client
				bConnect = true;

				return bConnect;
			}
			else {
				return false;
			}
		}
	}
}

//Close down UDP socket
bool MySocket::TerminateUDP() {
	if (connectionType != UDP) {
		//Prevent non-UDP setups
		return false;
	}
	else {
		bool ret = closesocket(ConnectionSocket) != SOCKET_ERROR;

		//Set connection status on successful termination
		if (ret) {
			ConnectionSocket = INVALID_SOCKET;
			bConnect = false;
		}

		return ret;
	}
}

/* DATA TRANSFER FUNCTIONS */

//All-inclusive send data over socket
int MySocket::SendData(const char* data, int size) {
	unsigned int rx = 0;

	//Send data differently depending on socket type
	if (connectionType == TCP) {
		if (mySocket == SERVER) {
			rx = send(ConnectionSocket, data, size, 0);
		}
		else if (mySocket == CLIENT) {
			rx = send(WelcomeSocket, data, size, 0);
		}
	}
	else if (connectionType == UDP) {
		if (mySocket == SERVER) {
			rx = sendto(ConnectionSocket, data, size, 0, (struct sockaddr*)&RespAddr, sizeof(RespAddr));
		}
		else if (mySocket == CLIENT) {
			SvrAddr.sin_family = AF_INET;
			SvrAddr.sin_port = htons(Port);
			SvrAddr.sin_addr.s_addr = inet_addr(IPAddr.c_str());

			rx = sendto(ConnectionSocket, data, size, 0, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
		}
	}

	return rx;
}

//All-inclusive get data from socket
int MySocket::GetData(char* data) {
	unsigned int rx = 0;

	//Clear the buffer before storing more into it (just in case there was something there before)
	memset(Buffer, 0, MaxSize);

	//Receive data differently depending on socket type
	if (connectionType == TCP) {
		if (mySocket == SERVER) {
			rx = recv(ConnectionSocket, Buffer, MaxSize, 0);
		}
		else if (mySocket == CLIENT) {
			rx = recv(WelcomeSocket, Buffer, MaxSize, 0);
		}

		//User must ensure data has enough space to copy to
		memcpy(data, Buffer, rx);
	}
	else if (connectionType == UDP) {
		int respLen = sizeof(RespAddr);
		rx = recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (struct sockaddr *)&RespAddr, &respLen);

		//User must ensure data has enough space to copy to
		memcpy(data, Buffer, rx);
	}

	return rx;
}

/* GETTERS AND SETTERS */

//Get IP Address
std::string MySocket::GetIPAddr() {
	return this->IPAddr;
}

//Set IP Address after ensuring socket isn't active
bool MySocket::SetIPAddr(std::string IP) {
	//Only set if there is no active socket(s)
	if (bConnect) {
		return false;
	}
	else {
		IPAddr = IP;
		return true;
	}
}

//Get Port number
int MySocket::GetPort() {
	return this->Port;
}

//Set Port number after ensuring socket isn't active
bool MySocket::SetPortNum(int portNum) {
	//Only set if there is no active socket(s)
	if (bConnect) {
		return false;
	}
	else {
		Port = portNum;
		return true;
	}
}

//Get the socket type (Client/Server)
SocketType MySocket::GetType() {
	return this->mySocket;
}

//Set the socket type (Client/Server) after ensuring socket isn't active
bool MySocket::SetType(SocketType socket) {
	if (bConnect) {
		return false;
	}
	else {
		mySocket = socket;
		return true;
	}
}