#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
/*
   _____  _  _               _   
  / ____|| |(_)             | |  
 | |     | | _   ___  _ __  | |_ 
 | |     | || | / _ \| '_ \ | __|
 | |____ | || ||  __/| | | || |_ 
  \_____||_||_| \___||_| |_| \__|
                                                         
*/
int socket(int domain, int type, int protocol);
int bind (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
int buildPacket(unsigned char packet[], unsigned int port, unsigned char data[], int relative, unsigned int overall);
int printPacket(unsigned char packet[]);
void connectionInit();
void parent();
void child();
void pipeInit();

int PACKET_SIZE = 10;
int DATA_LEN = 62;
int PACKET_BASE = 20761;

//IP of your machine.
char* myIP = "192.168.254.5";

struct sockaddr_in ControlAddr, FirstSubFlowAddr, SecondSubFlowAddr, ThirdSubFlowAddr;
struct in_addr in;

//Data to be sent
char* data = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
int totalLength = 988;

int controlSock, firstSock, secondSock, thirdSock, forkId;
int socketArr[3];
int childArr[3];
struct sockaddr_in addresses[3];
int forkCount = 0;
int pipeArr[3][2];
int childPrintCount = 0;
int exitFlag = 0;
int prod = 1;
/*
  __  __         _        
 |  \/  |       (_)       
 | \  / |  __ _  _  _ __  
 | |\/| | / _` || || '_ \ 
 | |  | || (_| || || | | |
 |_|  |_| \__,_||_||_| |_|

*/
//Forks the process and sends parents and children into their handlers.
int main(int argc, char *argv[])
{
	pipeInit();
	connectionInit();
	
	for(forkCount=0; forkCount<3; forkCount++){
		forkId = fork();
		if(forkId == 0){//child
			break;
		}else{
			childArr[forkCount] = forkId;
		}
	}
	
	if(forkId != 0){//parent
		parent();
	}else{
		child();
	}
}
/*
  _____  _                   _____         _  _   
 |  __ \(_)                 |_   _|       (_)| |  
 | |__)| _  _ __    ___       | |   _ __   _ | |_ 
 |  ___/| || '_ \  / _ \      | |  | '_ \ | || __|
 | |    | || |_) ||  __/     _| |_ | | | || || |_ 
 |_|    |_|| .__/  \___|    |_____||_| |_||_| \__|
           | |                                    
           |_|                                    
*/
//Initalizes the the pipes used to communicate between processes.
void pipeInit()
{
	pipe(pipeArr[0]);
	pipe(pipeArr[1]);
	pipe(pipeArr[2]);
}
/*
   _____  _      _  _      _ 
  / ____|| |    (_)| |    | |
 | |     | |__   _ | |  __| |
 | |     | '_ \ | || | / _` |
 | |____ | | | || || || (_| |
  \_____||_| |_||_||_| \__,_|
                             
*/
//Handles the child processes. Specifically, this function reads from its respective pipe and then 
//writes to the socket connection.
void child()
{
	childConnect(forkCount);
	close(pipeArr[forkCount][1]);
		while(exitFlag == 0){
			childPrintCount++;
			char str[11];
			read(pipeArr[forkCount][0], str, 10);
			if(prod){
				write(socketArr[forkCount], str, 10);//could be replaced with send
			}
			usleep(1000);//makes it look better
			//printPacket(str);
			//send/write
			if((forkCount != 2 && childPrintCount == 83) || (forkCount == 2 && childPrintCount == 82)) exitFlag = 1;
		}

		close(pipeArr[forkCount][0]);
		close(socketArr[forkCount]);
}

int childConnect(int child){

	if(prod){
		if (connect(socketArr[child], (struct sockaddr *)&addresses[child], sizeof(addresses[child])) < 0) {
				perror("ERROR binding cont");
				exit(1);
		}
	}


}
/*
  _____                          _   
 |  __ \                        | |  
 | |__) |__ _  _ __  ___  _ __  | |_ 
 |  ___// _` || '__|/ _ \| '_ \ | __|
 | |   | (_| || |  |  __/| | | || |_ 
 |_|    \__,_||_|   \___||_| |_| \__|
                                     
*/
//Handles the parent process. Specifically, the parent calls the packet creation and then pipes
//that packet to the child.
void parent()
{

		
		unsigned int totalSequence = 0;
		unsigned int whichSubFlow = 1;

		unsigned char current[4];
		unsigned char packet[PACKET_SIZE];

		int subFlow[3] = {0, 0, 0}; 
		int dataCount = 0;
		int i = 0;
		int repCount = 1;
		int port = 0;

		FILE *clientLog = fopen("clientlog.txt", "w");
		if(clientLog==NULL){
			printf("Error opening client log file.");
			exit(1);
		}
	
		
		
		
		close(pipeArr[whichSubFlow-1][0]);
		while(totalSequence<=totalLength){	
			for(i = 0; i < 4; i++, dataCount++){
				if(dataCount == DATA_LEN){
					dataCount = 0;
					repCount++;
				}
				current[i] = data[dataCount];
				fprintf(clientLog, "Byte %d on SubFlow %d with SubFlow Sequence Number %d\n", totalSequence+i, 						whichSubFlow, subFlow[whichSubFlow-1]);
			}

			port = whichSubFlow + PACKET_BASE - 1;

			buildPacket(packet, port, current, subFlow[whichSubFlow-1], totalSequence);
			
			usleep(500);
			write(pipeArr[whichSubFlow-1][1], packet, PACKET_SIZE);

			totalSequence+=4;
			subFlow[whichSubFlow-1]+=4;
			whichSubFlow ++;
			if(whichSubFlow == 4) whichSubFlow = 1;
		
		}
		wait(NULL);
		usleep(1000);
		close(pipeArr[whichSubFlow-1][1]);
		close(controlSock);

}
/*
  ____        _ _     _   _____           _        _   
 |  _ \      (_) |   | | |  __ \         | |      | |  
 | |_) |_   _ _| | __| | | |__) |_ _  ___| | _____| |_ 
 |  _ <| | | | | |/ _` | |  ___/ _` |/ __| |/ / _ \ __|
 | |_) | |_| | | | (_| | | |  | (_| | (__|   <  __/ |_ 
 |____/ \__,_|_|_|\__,_| |_|   \__,_|\___|_|\_\___|\__|

*/
//Builds the packet.                                                                                                
int buildPacket(unsigned char packet[], unsigned int port, unsigned char data[], int relative, unsigned int overall)
{	
	int i, j;
	//Set type and sequence number
	packet[0] = port & (0xFF);
	packet[1] = port >> 8; // ACK or DATA(1)
	packet[2] = relative & (0xFF);
	packet[3] = relative >> 8;
	packet[4] = overall & (0xFF);
	packet[5] = overall >> 8;
	packet[6] = data[0];
	packet[7] = data[1];
	packet[8] = data[2];
	packet[9] = data[3];

	return 1;
}
/*
  _____      _       _     _____           _        _   
 |  __ \    (_)     | |   |  __ \         | |      | |  
 | |__) | __ _ _ __ | |_  | |__) |_ _  ___| | _____| |_ 
 |  ___/ '__| | '_ \| __| |  ___/ _` |/ __| |/ / _ \ __|
 | |   | |  | | | | | |_  | |  | (_| | (__|   <  __/ |_ 
 |_|   |_|  |_|_| |_|\__| |_|   \__,_|\___|_|\_\___|\__|
                                                        
*/     
//Prints a packet in a readable format.                                               
int printPacket(unsigned char packet[]){
	int i = 0;
	printf("Port: %d ", packet[0]|(packet[1]<<8));
	printf("Subflow Sequence: %d ", packet[2]|(packet[3]<<8));
	printf("Total Sequence: %d ", packet[4]|(packet[5] << 8));
	printf("Data: %c%c%c%c\n", packet[6],packet[7],packet[8],packet[9]); 
	return 1;
}
/*
   _____                                  _    _                    _____         _  _   
  / ____|                                | |  (_)                  |_   _|       (_)| |  
 | |      ___   _ __   _ __    ___   ___ | |_  _   ___   _ __        | |   _ __   _ | |_ 
 | |     / _ \ | '_ \ | '_ \  / _ \ / __|| __|| | / _ \ | '_ \       | |  | '_ \ | || __|
 | |____| (_) || | | || | | ||  __/| (__ | |_ | || (_) || | | |     _| |_ | | | || || |_ 
  \_____|\___/ |_| |_||_| |_| \___| \___| \__||_| \___/ |_| |_|    |_____||_| |_||_| \__|
                                                                                         
*/
//Initializes all the connections to be used in MPTCP.
void connectionInit(){

	ControlAddr.sin_family = AF_INET;
	ControlAddr.sin_port = htons(20760);
	ControlAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((controlSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket cont");
      		exit(1);
	}

	if(prod){
		if (connect(controlSock, (struct sockaddr *)&ControlAddr, sizeof(ControlAddr)) < 0) {
				perror("ERROR binding cont");
				exit(1);
		}
	}


	FirstSubFlowAddr.sin_family = AF_INET;
	FirstSubFlowAddr.sin_port = htons(20761);
	FirstSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((firstSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 1");
      		exit(1);
	}

	SecondSubFlowAddr.sin_family = AF_INET;
	SecondSubFlowAddr.sin_port = htons(20762);
	SecondSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((secondSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 2");
      		exit(1);
	}

	ThirdSubFlowAddr.sin_family = AF_INET;
	ThirdSubFlowAddr.sin_port = htons(20763);
	ThirdSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((thirdSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 3");
      		exit(1);
	}

	socketArr[0] = firstSock;
	socketArr[1] = secondSock;
	socketArr[2] = thirdSock;

	addresses[0] = FirstSubFlowAddr;
	addresses[1] = SecondSubFlowAddr;
	addresses[2] = ThirdSubFlowAddr;
}







