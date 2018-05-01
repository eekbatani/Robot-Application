//Ehsan Ekbatani

ROBOT COMMUNICATION APPLICATION

This application was designed to communicate with a robot via a socket initialized on a local port.
 
The application can communicate and initialize with TCP/IP and/or UDP protocols, and utilizes multithreading.

One thread is for communicating operation commands to the Robot by serializing data packets
and sending them via the established socket connection to the robot,

Another thread is used to receive and deserialize incoming telemetry packets from the robot and output the values
to the console.

//Additional Notes

Main Command Menu:

1) DRIVE
-- 1 FORWARD
-- 2 BACKWARD
-- 3 RIGHT
-- 4 LEFT

Then enter the duration in seconds (between 0 and 255)

2) SLEEP
-- No Options

3) ARM
-- 5 UP
-- 6 DOWN

4) CLAW
-- 7 OPEN
-- 8 CLOSE

Error Injection Menu:

After each command is configured, the program will ask if you want to inject error.

0) DO NOT INJECT
1) INJECT BAD CRC
2) INJECT BAD LENGTH
3) INJECT BAD COMMAND FLAGS (set to 29 = 0001 1101, all cm