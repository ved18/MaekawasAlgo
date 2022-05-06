#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
using namespace std;

#define DONE "done"
#define ENQUIRY "enquiry"

#define REQUEST "request"
#define REPLY "reply"
#define YIELD "yield"
#define INQUIRE "inquire"
#define FAILED "failed"
#define RELEASE "release"

#define COMREQ "commitrequest"
#define AGREED "agreed"
#define ABORT "abort"
#define FINALABORT "finalabort"
#define COMMIT "commit"

#define PORT 11111 //defining temporary port
#define SA struct sockaddr
#define MAX 256

void startServer(int &serverSocket)
{
	struct sockaddr_in servaddr;

	//creating a socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		printf("\nCouldnt create socket");
		exit(0);
	}
	else
	{
		//printf("\nSocket creation successful");
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	//binding server to socket
	if ((bind(serverSocket, (SA *)&servaddr, sizeof(servaddr)) != 0))
	{
		printf("\nSocket bind failed");
		exit(0);
	}
	else
	{
		//printf("\nSocket binded");
	}
}

void connServer(int &fileD, const char *ip, int port)
{
	struct sockaddr_in servaddr;
	// cout << "\nDeets: " << ip << " " << port << endl;
	// creating socket
	fileD = socket(AF_INET, SOCK_STREAM, 0);
	if (fileD == -1)
	{
		cout << "\nfailed to connect to socket";
		exit(0);
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ip);
	// establishing connection with the server
	if (connect(fileD, (const SA *)&servaddr, sizeof(servaddr)) != 0)
	{
		fileD = -1;
		cout << "\nCould not connect to server: " << ip << "\nCheck if server is running." << endl;
	}
	else
	{
		//cout << "\nConnected to the server successfull." << endl;
	}
	return;
}