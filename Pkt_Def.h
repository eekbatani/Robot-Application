//BTN415 - MS3
//Ehsan Ekbatani, Sina Lahsaee, Cheng-Tuo Shueh

#ifndef _PKT_DEF_H_
#define _PKT_DEF_H_

#define FORWARD 1
#define BACKWARD 2
#define RIGHT 3
#define LEFT 4
#define UP 5
#define DOWN 6
#define OPEN 7
#define CLOSE 8
#define HEADERSIZE 6

enum CmdType { NONE, DRIVE, SLEEP, ARM, CLAW, ACK };

struct Header {
	int PktCount = 0;

	unsigned char Sleep		: 1;
	unsigned char Status	: 1;
	unsigned char Drive		: 1;
	unsigned char Claw		: 1;
	unsigned char Arm		: 1;

	unsigned char Ack		: 1;
	unsigned char Padding	: 2;

	unsigned char Length;
};

struct MotorBody
{
	unsigned char Direction;
	unsigned char Duration;
};

struct ActuatorBody
{
	unsigned char Action;
};

class PktDef {
private:
	struct CmdPacket {
		Header Head;
		char* Data;
		char CRC;
	} Packet;

	char* RawBuffer;

public:
	PktDef();
	PktDef(char*);
	~PktDef();
	void SetCmd(CmdType);
	void SetBodyData(char*, int);
	void SetPktCount(int);
	CmdType GetCmd();
	bool GetStatus();
	bool GetAck();
	int GetLength();
	char* GetBodyData();
	int GetPktCount();
	bool CheckCRC(char*, int);
	void CalcCRC();
	char* GenPacket();
};

#endif