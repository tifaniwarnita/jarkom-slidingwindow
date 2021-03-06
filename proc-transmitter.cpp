/* 
 * File 		: transmitter.h
 * Author 		: Ahmad Naufal (049) - Tifani Warnita (055) - Asanilta Fahda (079)
 * Description	: Header of transmitter
 */ 

#include "ack.h"
#include "frame.h"
#include "queue.h"
#include "window.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>

using namespace std;

char *receiverIP;
FILE *tFile;
int port;

// socket attribute
int sockfd;
struct hostent *server;
struct sockaddr_in receiverAddr;
bool isSocketOpen;	

Frame *frameStorage;
int fcount;
int received;
pthread_t thread[1];
Window window;

//Queue slidingWindow(WINSIZE);
void error(const char *message);
void* childProcessACK(void *threadid);

int main(int argc, char *argv[])
{
	pthread_t thread[1];

	cout << "Haha kobam" << endl;
	if (argc < 4) {
		// case if arguments are less than specified
		printf("Please use the program with arguments: %s <target-ip> <port> <filename>\n", argv[0]);
		return 0;
	} else {
		receiverIP = argv[1];
		frameStorage = new Frame[50];
		port = atoi(argv[2]);

		tFile = fopen(argv[3], "r");
		if (tFile == NULL) 
			error("ERROR: File text not Found.\n");

		if ((server = gethostbyname(receiverIP)) == NULL)
			error("ERROR: Receiver Address is Unknown or Wrong.\n");
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			error("ERROR: Create socket failed.\n");

		isSocketOpen = true;

		memset(&receiverAddr, 0, sizeof(receiverAddr));
		receiverAddr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&receiverAddr.sin_addr.s_addr, server->h_length);
		receiverAddr.sin_port = htons(port);
		printf("Socket has been created at %s:%d\n", receiverIP, port);

		fcount = 0;
		int i = 0;

		char c;
		char* tempdata = new char[DATAMAX+1];

		do {
			c = fgetc(tFile);
			
			if (i==DATAMAX) {
				tempdata[i] = '\0';
				i=0;
				cout << "ini tempdata " << tempdata << endl;
				Frame temp(fcount%MAXSEQ,tempdata);
				frameStorage[fcount++] = temp; 
				cout << "berhasil bikin" << endl;		
			}

			if (c==EOF) {
				tempdata[i] = Endfile;
				tempdata[i+1] = '\0';
				Frame temp(fcount%MAXSEQ,tempdata);
				frameStorage[fcount] = temp; 
			} else {
				tempdata[i++] = c;
			}
			cout << i << " " << tempdata[i-1] << endl;
		} while (c!=EOF);

		cout << "berhasil baca file" << endl;

		if (pthread_create(&thread[0], NULL, childProcessACK, 0) != 0) 
			error("ERROR: Failed to create thread for child. Please free some space.\n");

		int framesent = 0; //number of frame sent
		int j = 0; //number of frame put to the window
		int timeCount = 0;
		received = 0;

		//Initialize the window
		while (!window.getFrameBuffer().isFull()) {
			if(j<=fcount) {
				cout << frameStorage[j].getData() << endl;
				window.addFrame(frameStorage[j], timeCount);
				timeCount += 1;
				j++;
			} else {
				break;
			}
		}

		//Sending the frame
		while (framesent<=fcount || received<fcount) {
			cout << "i: " << framesent << " received:" << received << " fcount:" << fcount << endl;
			for(int x=0; x<window.getFrameBuffer().getCount(); x++) {
				int a = (x+window.getFrameBuffer().getHead())%WINSIZE; // Ini ga efektif banget maaf ya
				if(window.getTimeOut(a) == 0) {
					cout << "Mencoba mengirim: " << window.getFrameBuffer().getElement(a).getSerialized() << endl;
					if (sendto(sockfd, window.getFrameBuffer().getElement(a).getSerialized(), window.getFrameBuffer().getElement(a).getSize(), 0, (const struct sockaddr *) &receiverAddr, sizeof(receiverAddr)) == -1)
						error("ERROR: sendto() sent buffer with size more than expected.\n");
					printf("Sending frame no. %d: %s\n", window.getFrameBuffer().getElement(a).getNo(), window.getFrameBuffer().getElement(a).getData());
					framesent++;
					window.setTimeOut(a);
				}
			}
			//Reduce all timeout
			window.reduceTimeOut();

			sleep(1);
			//Slide window
			int a=0;
			while (a<WINSIZE) {
				if (window.getIsAck(window.getFrameBuffer().getHead()+a) == true) {
					window.slideWindow(frameStorage[j]);
					j++;
				} else {
					break;
				}
			}
		}

		pthread_join(thread[0], NULL);
		close(sockfd);
		isSocketOpen = false;
	}

	return 0;
}

void error(const char *message) {
	perror(message);
	exit(1);
}

void* childProcessACK(void *threadid) {
	// child process
	// receive ACK/NAK from receiver
	struct sockaddr_in srcAddr;
	socklen_t srcLen;
	printf("masuk thread\n");

	char* serializedAck;

	 while (isSocketOpen) {
		printf("proses\n");
		if (recvfrom(sockfd, serializedAck, 4, 0, (struct sockaddr *) &srcAddr, &srcLen) != sizeof(Ack))
			error("ERROR: recvfrom() receive buffer with size more than expected.\n");

		Ack *ack = new Ack(serializedAck);
		int i=0;
		while (i<WINSIZE) {
			if (window.getFrameBuffer().getElement((window.getFrameBuffer().getHead()+i)%WINSIZE).getNo() == ack->getFrameNo())
				break;
			else
				i++;
		}

		if (ack->getAck()==ACK) {
			printf("Received ACK for Frame No: %d\n",ack->getFrameNo());
			window.setAckTrue((window.getFrameBuffer().getHead()+i)%WINSIZE);
			received++;
		} else if (ack->getAck()==NAK) {
			printf("Received NAK for Frame No: %d\n",ack->getFrameNo());
			if (sendto(sockfd, window.getFrameBuffer().getElement(i).getSerialized(), window.getFrameBuffer().getElement(i).getSize(), 0, (const struct sockaddr *) &receiverAddr, sizeof(receiverAddr)) != window.getFrameBuffer().getElement(i).getSize())
					error("ERROR: sendto() sent buffer with size more than expected.\n");
			printf("Sending frame no. %d: %s\n", window.getFrameBuffer().getElement(i).getNo(), window.getFrameBuffer().getElement(i).getData());
			window.setTimeOut(i);
		}		
	} 

	pthread_exit(NULL);
}