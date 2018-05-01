//BTN415 - MS3
//Ehsan Ekbatani, Sina Lahsaee, Cheng-Tuo Shueh

#include <cstring>
#include "Pkt_Def.h"

//Default constructor
PktDef::PktDef()
{
	Packet.Head.PktCount = 0;

	Packet.Head.Drive = 0;
	Packet.Head.Status = 0;
	Packet.Head.Sleep = 0;
	Packet.Head.Arm = 0;
	Packet.Head.Claw = 0;
	Packet.Head.Ack = 0;
	Packet.Head.Padding = 0;

	Packet.Head.Length = 0;

	Packet.Data = nullptr;
	Packet.CRC = 0;
	RawBuffer = nullptr;
}

PktDef::PktDef(char* buffer) {
	//Get header - PktCount
	memcpy(&Packet.Head.PktCount, &buffer[0], 4);

	//Get header - Flags
	Packet.Head.Sleep = buffer[4] & 1;
	Packet.Head.Status = (buffer[4] >> 1) & 1;
	Packet.Head.Drive = (buffer[4] >> 2) & 1;
	Packet.Head.Claw = (buffer[4] >> 3) & 1;
	Packet.Head.Arm = (buffer[4] >> 4) & 1;
	Packet.Head.Ack = (buffer[4] >> 5) & 1;

	//Get header - Length
	memcpy(&Packet.Head.Length, &buffer[5], 1);

	//Calculate the bodysize
	unsigned int bodySize = (Packet.Head.Length - HEADERSIZE - 1);

	//Check for 0-length body
	if (bodySize != 0) {
		//Allocate memory for frame body based on retrieved length
		Packet.Data = new char[bodySize];

		//Get the data into struct body
		memcpy(&Packet.Data[0], &buffer[HEADERSIZE], bodySize);

		//Get data into struct tail
		memcpy(&Packet.CRC, &buffer[HEADERSIZE + bodySize], 1);
	}
	else {
		Packet.Data = nullptr;
	}

	RawBuffer = nullptr;
}

PktDef::~PktDef() {
	delete[] RawBuffer;
	RawBuffer = nullptr;

	delete[] Packet.Data;
	Packet.Data = nullptr;
}

void PktDef::SetCmd(CmdType command)
{
	switch (command) {
	case DRIVE:
		Packet.Head.Drive = 1;
		Packet.Head.Sleep = 0;
		Packet.Head.Arm = 0;
		Packet.Head.Claw = 0;
		Packet.Head.Ack = 0;

		break;
	case SLEEP:
		Packet.Head.Drive = 0;
		Packet.Head.Sleep = 1;
		Packet.Head.Arm = 0;
		Packet.Head.Claw = 0;
		Packet.Head.Ack = 0;

		break;
	case ARM:
		Packet.Head.Drive = 0;
		Packet.Head.Sleep = 0;
		Packet.Head.Arm = 1;
		Packet.Head.Claw = 0;
		Packet.Head.Ack = 0;

		break;
	case CLAW:
		Packet.Head.Drive = 0;
		Packet.Head.Sleep = 0;
		Packet.Head.Arm = 0;
		Packet.Head.Claw = 1;
		Packet.Head.Ack = 0;

		break;
	case ACK:
		Packet.Head.Ack = 1;

		break;
	}
}

void PktDef::SetBodyData(char* body, int size) {
	//Allocate + deep copy of body data
	delete[] Packet.Data;
	Packet.Data = nullptr;
	Packet.Data = new char[size];

	for (int i = 0; i < size; ++i) {
		Packet.Data[i] = body[i];
	}

	//Set the full packet size
	Packet.Head.Length = HEADERSIZE + size + 1;
}

void PktDef::SetPktCount(int count) {
	Packet.Head.PktCount = count;
}

CmdType PktDef::GetCmd() {
	if (Packet.Head.Sleep)
	{
		return SLEEP;
	}
	else if (Packet.Head.Drive)
	{
		return DRIVE;
	}
	else if (Packet.Head.Claw)
	{
		return CLAW;
	}
	else if (Packet.Head.Arm)
	{
		return ARM;
	}
	else
	{
		return NONE;
	}
}

bool PktDef::GetStatus() {
	return Packet.Head.Status;
}

bool PktDef::GetAck() {
	return Packet.Head.Ack;
}

int PktDef::GetLength() {
	return (unsigned int)Packet.Head.Length;
}

char* PktDef::GetBodyData() {
	return Packet.Data;
}

int PktDef::GetPktCount() {
	return Packet.Head.PktCount;
}

bool PktDef::CheckCRC(char* buffer, int size) {
	bool ret = false;
	int onesCount = 0;
	char* singleByte = buffer;

	//Count the number of 1 bits in buffer excluding CRC
	//Outer loop: bytes
	for (int i = 0; i < size - 1; ++i) {
		//Inner loop: bits
		for (int j = 0; j < 8; j++) {
			//Shift to LSB and mask with 1 to check if bit set
			onesCount += (*singleByte >> j) & 1;
		}

		//Move byte pointer forward to next address
		singleByte += 1;
	}

	if (onesCount == buffer[size - 1]) {
		ret = true;
	}

	return ret;
}

void PktDef::CalcCRC() {
	int onesCount = 0;
	int bodySize = Packet.Head.Length - HEADERSIZE - 1;
	char* singleByte = (char*)&Packet.Head;

	//Count the number of 1 bits in header up to flags
	//Outer loop: bytes
	for (int i = 0; i < 5; ++i) {
		//Inner loop: bits
		for (int j = 0; j < 8; j++) {
			//Shift to LSB and mask with 1 to check if bit set
			onesCount += (*singleByte >> j) & 1;
		}

		//Move byte pointer forward to next address
		singleByte += 1;
	}

	//Count the number of 1 bits in header length attribute
	singleByte = (char*)&Packet.Head.Length;

	for (int j = 0; j < 8; j++) {
		onesCount += (*singleByte >> j) & 1;
	}

	//Counter the number of 1 bits in body
	singleByte = (char*)Packet.Data;

	for (int i = 0; i < bodySize; ++i) {
		for (int j = 0; j < 8; j++) {
			onesCount += (*singleByte >> j) & 1;
		}

		singleByte += 1;
	}

	Packet.CRC = onesCount;
}

char* PktDef::GenPacket() {
	//preventing memory leaks;
	delete[] RawBuffer;
	RawBuffer = nullptr;

	unsigned int bodySize = (Packet.Head.Length - HEADERSIZE - 1);

	//Allocate enough memory for buffer
	RawBuffer = new char[Packet.Head.Length];

	//Header - PktCount
	memcpy(&RawBuffer[0], &Packet.Head.PktCount, 4);

	//Header - Flags
	memcpy(&RawBuffer[4], &Packet.Head.PktCount + 1, 1);

	//Header - Length
	memcpy(&RawBuffer[5], &Packet.Head.Length, 1);

	//Body information
	memcpy(&RawBuffer[HEADERSIZE], Packet.Data, bodySize);

	//Tail information
	memcpy(&RawBuffer[HEADERSIZE + bodySize], &Packet.CRC, 1);

	return RawBuffer;
}