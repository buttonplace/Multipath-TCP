#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
/*
Server
                                                         
*/
int socket(int domain, int type, int protocol);
int bind (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
int buildPacket(unsigned char packet[], unsigned int port, unsigned char data[], int relative, unsigned int overall);
int printPacket(unsigned char packet[]);
void connectionInit();
void parent();
void child();
void pipeInit();
int childAccept();
int parentAccept();

int PACKET_SIZE = 10;

char* myIP = "192.168.254.5";

struct sockaddr_in ControlAddr, FirstSubFlowAddr, SecondSubFlowAddr, ThirdSubFlowAddr;
struct in_addr in;

int controlSock, firstSock, secondSock, thirdSock, acceptControlSock, forkId;
int childAcceptSocketArr[3];

int socketArr[3];
int childArr[3];

struct sockaddr_in addresses[3];
struct sockaddr_in clientAddresses[3];
int clientAddressesLen[3];
int ControlAddrLen;

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
//Handles the child processes. Specifically, this function reads from its socket and then writes to the pipe
//so the parent can get the data.
void child()
{
	childAccept(forkCount);
	close(pipeArr[forkCount][0]);
		while(exitFlag == 0){
			childPrintCount++;
			char str[11];
			read(childAcceptSocketArr[forkCount], str, 10);
			
			write(pipeArr[forkCount][1], str, 10);

			usleep(1000);//makes it look better
			//printPacket(str);
			if((forkCount != 2 && childPrintCount == 83) || (forkCount == 2 && childPrintCount == 82)) exitFlag = 1;
		}

		close(pipeArr[forkCount][1]);
		close(childAcceptSocketArr[forkCount]);
}

int childAccept(int child){

	if(prod){
		if (listen(socketArr[child], 5)) {
				perror("ERROR listen child");
				printf("%d", child);
				exit(1);
		}
		if((childAcceptSocketArr[child] = accept(socketArr[child], (struct sockaddr *)&clientAddresses[child], 				&clientAddressesLen[child])) < 0){
				perror("ERROR accept child");
				printf("%d", child);
				exit(1);
		}
	}
	

}

//Byte 0-3 -> SubFlow 1, SubFlow Sequence 0, DATA: 0123


/*
  _____                          _   
 |  __ \                        | |  
 | |__) |__ _  _ __  ___  _ __  | |_ 
 |  ___// _` || '__|/ _ \| '_ \ | __|
 | |   | (_| || |  |  __/| | | || |_ 
 |_|    \__,_||_|   \___||_| |_| \__|
                                     
*/
//Handles the parent process. Specifically, the parent reads from each of the three pipes and writes to the log file.
void parent()
{

	parentAccept();

	unsigned int whichSubFlow = 1;
	unsigned char packet[PACKET_SIZE];
	int subFlow[3] = {0, 0, 0};
	unsigned int totalSequence = 0;
	int i = 0;

	FILE *serverLog = fopen("serverLog.txt", "w");
	if(serverLog==NULL){
		printf("Error opening server log file.");
		exit(1);
	}

	

		//subflowseq*3 + ((port-20760)-1)*4
		//totalSeq = (packet[2]|(packet[3]<<8))*3 + (packet[0]|(packet[1]<<8))-20760)*4
		//we can do this because we are assuming that we know which connection is the first sublow, and the packets are sent in a cycle. (We can calculate the total sequence number using the port number and the subflow sequence number)
		close(pipeArr[whichSubFlow-1][1]);
		while(totalSequence<=988){
			read(pipeArr[whichSubFlow-1][0], packet, PACKET_SIZE);
			printPacket(packet);
			for(i = 0; i<4; i++){
				fprintf(serverLog, "SubFlow Sequence Number %d on SubFlow %d is Byte %d\n", (packet[2]|(packet	[3]<<8)), whichSubFlow, (packet[2]|(packet[3]<<8))*3 + ((packet[0]|(packet[1]<<8))-20761)*4 + i);
			}
			

			whichSubFlow ++;
			if(whichSubFlow == 4) whichSubFlow = 1;
			totalSequence+=4;
			subFlow[whichSubFlow-1]+=4;
				
		}
		
	
		wait(NULL);
		usleep(1000);
		close(pipeArr[whichSubFlow-1][0]);
		close(controlSock);

}

int parentAccept(){
	if(prod){
		if (listen(controlSock, 5)) {
				perror("ERROR listen parent");
				printf("%d", child);
				exit(1);
		}
		if((acceptControlSock = accept(controlSock, (struct sockaddr *)&ControlAddr, 						&ControlAddrLen)) < 0){
				perror("ERROR accept child");
				printf("%d", child);
				exit(1);
		}
	}
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

	if (bind(controlSock, (struct sockaddr *)&ControlAddr, sizeof(ControlAddr)) < 0) {
	      		perror("ERROR binding cont");
	      		exit(1);
	}

	FirstSubFlowAddr.sin_family = AF_INET;
	FirstSubFlowAddr.sin_port = htons(20761);
	FirstSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((firstSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 1");
      		exit(1);
	}

	if (bind(firstSock, (struct sockaddr *)&FirstSubFlowAddr, sizeof(FirstSubFlowAddr)) < 0) {
	      	perror("ERROR binding 1");
	      	exit(1);
	}
	
	SecondSubFlowAddr.sin_family = AF_INET;
	SecondSubFlowAddr.sin_port = htons(20762);
	SecondSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((secondSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 2");
      		exit(1);
	}

	if (bind(secondSock, (struct sockaddr *)&SecondSubFlowAddr, sizeof(SecondSubFlowAddr)) < 0) {
	      	perror("ERROR binding 2");
	      	exit(1);
	}

	ThirdSubFlowAddr.sin_family = AF_INET;
	ThirdSubFlowAddr.sin_port = htons(20763);
	ThirdSubFlowAddr.sin_addr.s_addr = inet_addr(myIP);

	if ((thirdSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      		perror("ERROR socket 3");
      		exit(1);
	}

	if (bind(thirdSock, (struct sockaddr *)&ThirdSubFlowAddr, sizeof(ThirdSubFlowAddr)) < 0) {
	      	perror("ERROR binding 3");
	      	exit(1);
	}
	
	socketArr[0] = firstSock;
	socketArr[1] = secondSock;
	socketArr[2] = thirdSock;

	addresses[0] = FirstSubFlowAddr;
	addresses[1] = SecondSubFlowAddr;
	addresses[2] = ThirdSubFlowAddr;
}





