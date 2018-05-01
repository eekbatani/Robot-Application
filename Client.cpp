//BTN415 - MS3
//Ehsan Ekbatani, Sina Lahsaee, Cheng-Tuo Shueh

#include <thread>
#include <iomanip>

#include "Pkt_Def.h"
#include "MySocket.h"

bool ExeComplete = false;

enum InjectError { NOERR, BADCRC, BADLENGTH, BADCMD };

void injectError(int err, char* buffer, unsigned int length) {
	char* ptr = nullptr;
	unsigned int bodySize = (length - HEADERSIZE - 1);

	switch (err) {
	case BADCRC:
		buffer[HEADERSIZE + bodySize] += 1;
		break;

	case BADLENGTH:
		buffer[5] += 1;
		break;

	case BADCMD:
		buffer[4] = 29;
		break;
	}
}

//Command thread logic
void command(std::string ipAddr, int portNum)
{
	//-- SETTING UP --//

	//start running the input loop
	bool run = true;

	//buffer for receiving robot responses
	char* rxBuffer = nullptr;

	//variables for capturing user input
	int input;
	MotorBody motor;
	ActuatorBody arm; 
	ActuatorBody claw;
	unsigned int direction;
	unsigned int duration;
	
	//create packet structs for sending commands and receiving from robot
	PktDef tPacket;

	//use macros from previous milestones for assigning types
	SocketType client = CLIENT;
	ConnectionType tcp = TCP;

	//create socket and connect to server
	MySocket cmdSocket(client, ipAddr, portNum, tcp, DEFAULT_SIZE);
	bool succ = cmdSocket.ConnectTCP();

	while (run && succ) 
	{
		//Query the user in order to get all required information to form a packet, as defined in PktDef
		//always refer to macros from MS1 

		input = NONE;

		//loop while there is no valid command entry
		while (input < DRIVE || input > CLAW) 
		{
			std::cout << "Enter the commmand number:"
							"\n1 DRIVE"
							"\n2 SLEEP"
							"\n3 ARM"
							"\n4 CLAW"
							<< std::endl << "> ";

			//Get the user input
			std::cin >> input;

			//handle invalid input
			if (input < DRIVE || input > CLAW)
			{
				std::cout << "ERROR: Invalid Command." << std::endl;
			}
		}

		std::cout << std::endl;
		
		//proceed to handle valid command entry
		switch (input) 
		{
			case DRIVE: 
				direction = 0;
				duration = -1;

				//use same menu logic as earlier
				while (direction < FORWARD || direction > LEFT) 
				{
					std::cout << "Enter the direction number:"
									"\n1 Forward"
									"\n2 Backward"
									"\n3 Right"
									"\n4 Left"
									<< std::endl << "> ";

					//Get user input for direction
					std::cin >> direction;

					std::cout << std::endl;

					//handle invalid input
					if (direction < FORWARD || direction > LEFT) 
					{
						std::cout << "ERROR: Invalid Direction." << std::endl;
					}
				}

				while (duration < 0 || duration > 255)
				{
					//Get user input for duration
					std::cout << "Enter the duration (seconds): ";
					std::cin >> duration;

					//handle invalid input
					if (duration < 0 || duration > 255)
					{
						std::cout << "ERROR: Duration must be between 0 and 255 seconds" << std::endl;
					}
				}
				
				//Configure command after everything is verified
				motor.Direction = direction;
				motor.Duration = duration;

				//Configure the Packet for DRIVE
				tPacket.SetCmd(DRIVE);
				tPacket.SetBodyData((char *)&motor, 2);

				break;

			case SLEEP:
				//Configure the Packet for SLEEP
				tPacket.SetCmd(SLEEP);
				tPacket.SetBodyData(nullptr, 0); //SLEEP packet should have a 0-length body

				break;
			
			case ARM:
				direction = 0;

				while (direction < UP || direction > DOWN) 
				{
					std::cout << "Enter Arm Action:"
									"\n5 Up"
									"\n6 Down"
									<< std::endl << "> ";

					//Get user input for direction
					std::cin >> direction;

					//handle invalid input
					if (direction < UP || direction > DOWN) 
					{
						std::cout << "ERROR: Invalid Action.";
					}
				}

				//Configure command after everything is verified
				arm.Action = direction;

				//Configure the Packet
				tPacket.SetCmd(ARM);
				tPacket.SetBodyData((char *)&arm, 1);

				break;
	
			case CLAW:
				direction = 0;

				while (direction < OPEN || direction > CLOSE)
				{
					std::cout << "Enter Claw Action:"
									"\n7 Open"
									"\n8 Close"
									<< std::endl << "> ";

					std::cin >> direction;

					if (direction < OPEN || direction > CLOSE)
					{
						std::cout << "ERROR: Invalid Action.";
					}
				}

				claw.Action = direction;

				//Configure the Packet
				tPacket.SetCmd(CLAW);

				//pass in address of the body for this one i think?
				tPacket.SetBodyData((char *)&claw, 1);

				break;
		}

		std::cout << std::endl;

		//-- SEND PACKET AFTER CONFIGURING --//
		//Increment the PktCount
		tPacket.SetPktCount(tPacket.GetPktCount() + 1);

		//Calculate CRC
		tPacket.CalcCRC();

		//Generate the raw packet to send
		char* raw = tPacket.GenPacket();
		int errInjectInput = 0;

		//-- ERROR INJECTION --//
		do 
		{
			std::cout << "Inject an error into data?:"
				"\n0 Do not inject"
				"\n1 Inject BAD CRC"
				"\n2 Inject BAD LENGTH"
				"\n3 Inject BAD COMMAND"
				<< std::endl << "> ";

			//Get user input for direction
			std::cin >> errInjectInput;

			std::cout << std::endl;

			//handle invalid input
			if (errInjectInput < NOERR || errInjectInput > BADCMD)
			{
				std::cout << "ERROR: Invalid Error Injection Type." << std::endl;
			}
		} while (errInjectInput < NOERR || errInjectInput > BADCMD);

		//Inject the error if user wanted to
		if (errInjectInput != NOERR) {
			injectError(errInjectInput, raw, tPacket.GetLength());
		}

		//-- SEND THE DATA OVER --//
		cmdSocket.SendData(raw, tPacket.GetLength());

		//-- RECEIVE RESPONSE FROM ROBOT --//
		//reset the buffer
		delete[] rxBuffer;
		rxBuffer = nullptr;
		rxBuffer = new char[DEFAULT_SIZE];

		//receive data into buffer
		cmdSocket.GetData(rxBuffer);

		//store serialized raw data into packet object
		PktDef rPacket(rxBuffer);

		//First check CRC, proceed only if correct
		if (rPacket.CheckCRC(rxBuffer, rPacket.GetLength())) {

			//NACK response: no commands set and no ACK
			if (rPacket.GetCmd() == NONE && !rPacket.GetAck())
			{
				std::cout << "Error: Received NACK from robot!" << std::endl;
			}
			//ACK properly set
			else if (rPacket.GetAck())
			{
				std::cout << "Recieved ACK from robot for command: " << rPacket.GetCmd() << std::endl;

				//If it was a SLEEP ACK, disconnect
				if (rPacket.GetCmd() == SLEEP) {
					run = false;
					cmdSocket.DisconnectTCP();
				}
			}
			//ACK not properly set
			else
			{
				std::cout << "Error: ACK not detected! Command was: " << rPacket.GetCmd() << std::endl;
			}

		}
		else {
			std::cout << "Error: CRC mismatch, possible packet corruption" << std::endl;
		}

		std::cout << std::endl;
	}

	std::cout << "Ending Client command thread" << std::endl;

	ExeComplete = true;
}

//Telemetry thread logic
void telemetry(std::string ipAddr, int portNum)
{
	//-- SETTING UP --//
	//telemetry struct
	struct {
		short sonar = 0;
		short armPos = 0;
		
		unsigned char DriveFlag : 1;
		unsigned char ArmUp : 1;
		unsigned char ArmDown : 1;
		unsigned char ClawOpen : 1;
		unsigned char ClawClosed : 1;

		unsigned char Padding : 3;

	} teleBody;

	//start running the input loop
	bool run = true;

	//buffer for receiving robot responses
	char* rxBuffer = nullptr;

	//use macros from previous milestones for assigning types
	SocketType client = CLIENT;
	ConnectionType tcp = TCP;

	//create socket and connect to server
	MySocket telSocket(client, ipAddr, portNum, tcp, 0);
	bool succ = telSocket.ConnectTCP();

	while (run && succ)
	{
		//-- RECEIVE RESPONSE FROM ROBOT --//

		//reset the buffer
		delete[] rxBuffer;
		rxBuffer = nullptr;
		rxBuffer = new char[DEFAULT_SIZE];

		//receive data into buffer
		telSocket.GetData(rxBuffer);

		//store serialized raw data into packet object
		PktDef rPacket(rxBuffer);

		//First check CRC, proceed only if correct
		if (rPacket.CheckCRC(rxBuffer, rPacket.GetLength())) {

			//NACK response: no commands set and no ACK
			if (!rPacket.GetStatus()) {
				std::cout << "Error: Status not set in telemetry message!" << std::endl;
			}
			//ACK properly set
			else {
				//Print the RAW packet
				char* ptr = rxBuffer;
				std::cout << "RAW Body from telemetry: ";
				for (int i = 0; i < HEADERSIZE + 5 + 1; ++i) {
					std::cout << std::hex << (unsigned int)*(ptr++) << ", ";
				}

				std::cout << std::endl;

				/*
				//Print the RAW Header
				char* ptr = (char*)&rPacket;
				std::cout << "RAW Body from telemetry: ";
				for (int i = 0; i < HEADERSIZE; ++i) {
					std::cout << std::hex << (unsigned int)*(ptr++) << ", ";
				}

				//Print the RAW Body
				ptr = rPacket.GetBodyData();
				for (int i = 0; i < 5; ++i) {
					std::cout << std::hex << (unsigned int)*(ptr++) << ", ";
				}

				//Print the RAW Tail
				ptr = (char*)&rPacket + rPacket.GetLength();
				std::cout << std::hex << (unsigned int)*(ptr) << std::endl;
				*/

				//Fill telemetry struct with body info
				memcpy(&teleBody.sonar, &rPacket.GetBodyData()[0], 2);
				memcpy(&teleBody.armPos, &rPacket.GetBodyData()[2], 2);

				teleBody.DriveFlag = rPacket.GetBodyData()[4] & 1;
				teleBody.ArmUp = (rPacket.GetBodyData()[4] >> 1) & 1;
				teleBody.ArmDown = (rPacket.GetBodyData()[4] >> 2) & 1;
				teleBody.ClawOpen = (rPacket.GetBodyData()[4] >> 3) & 1;
				teleBody.ClawClosed = (rPacket.GetBodyData()[4] >> 4) & 1;

				//Pad with zeroes
				teleBody.Padding = 0;

				//Display readings to screen
				std::cout << "Sonar reading: " << std::endl << std::to_string((float)teleBody.sonar) << std::endl;
				std::cout << "Arm position reading: " << std::endl << std::to_string((float)teleBody.armPos) << std::endl;

				//Display flags in plain English
				std::cout << "Drive Status: " << std::string(teleBody.DriveFlag ? "DRIVING(1)" : "STOPPED(0)") << std::endl;

				if (teleBody.ArmUp) {
					std::cout << "Arm is up, ";
				}
				else if (teleBody.ArmDown) {
					std::cout << "Arm is down, ";
				}

				if (teleBody.ClawOpen) {
					std::cout << "Claw is open";
				}
				else if (teleBody.ClawClosed) {
					std::cout << "Claw is closed";
				}

				//If it was a SLEEP ACK, disconnect
				if (rPacket.GetCmd() == SLEEP) {
					run = false;
					telSocket.DisconnectTCP();
				}
			}

			std::cout << std::endl;

		}
		else {
			std::cout << "Error: CRC mismatch, possible packet corruption" << std::endl;
		}
	}
}

int main(int argc, char* argv)
{
	//variables for constructing socket
	std::string ipAddr;
	int CmdportNum, TelPortNum;

	//Query the user in order to get all required information to form generate mySocket
	std::cout << "Enter an IP Address: ";
	std::cin >> ipAddr;

	std::cout << "Enter a Port Number for the Command Thread: ";
	std::cin >> CmdportNum;

	std::cout << "Enter a Port Number for the Telemetry Thread: ";
	std::cin >> TelPortNum;

	std::cout << std::endl;

	std::thread(command, ipAddr, CmdportNum).detach();
	std::thread(telemetry, ipAddr, TelPortNum).detach();

	//this make sure main continues until connection with the robot is terminated
	while (!ExeComplete) {};

	std::cin.clear();
	std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

	std::cout << "Client program complete, press any key to exit.";
	std::cin.get();

	return 0;
}
