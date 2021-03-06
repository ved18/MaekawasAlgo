#include<bits/stdc++.h>
#include"connection.cpp"
using namespace std;

void sendFileList(int clientSocket)
{
    string temp = "";
    for (int i = 0; i <= 2; i++)
    {
        char *response = new char[MAX];
        temp = "TestFile";
        temp += to_string(i);
        //cout<<temp<<endl;
        memcpy(response, temp.data(), temp.length());
        send(clientSocket, response, MAX, 0);
        temp = "";
        delete response;
    }
    return;
}

void sockListenClient(int serverSocket)
{
    int clientSocket = -1;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    // accepting requests from another client
    clientSocket = accept(serverSocket, (SA *)&cliaddr, &len);
    if (clientSocket < 0)
    {
        cout << "\nFailed to connect to client." << endl;
        exit(0);
    }
    else
    {
        cout << "\nServer connected to client." << endl;
    }

    while (true)
    {
        char *request = new char[MAX];
        if (read(clientSocket, request, MAX) == -1)
        {
            cout << "\nError in reading from client" << endl;
            close(clientSocket);
            exit(0);
        }
        if (!strcmp(request, DONE))
        {
            close(clientSocket);
            exit(0);
        }
        else if (!strcmp(request, ENQUIRY)) // Enquiry about files
        {
            cout << "\nSending file list to server." << endl;
            thread sendList(sendFileList, clientSocket);
            sendList.join();
            send(clientSocket, DONE, MAX, 0);
        }
        else if(!strcmp(request, COMREQ))
        {
            char fileName[MAX];
            if(read(clientSocket, fileName, MAX) != -1)
            {
                if(!strcmp(fileName, "TestFile4"))
                {
                    if(send(clientSocket, ABORT, MAX, 0) != -1)
                    {
                        cout<<"\nSent abort to write."<<endl;
                    }
                }
                else
                {
                    if(send(clientSocket, AGREED, MAX, 0) != -1)
                    {
                        cout<<"\nSent agreed to write."<<endl;
                    }
                }
                    
            }
        }
        else if(!strcmp(request, FINALABORT))
        {
            cout<<"\nReceived abort from client."<<endl;
        }
        else if(!strcmp(request, COMMIT))
        {
                cout<<"\nReceived commit from the client"<<endl;
        }
        else
        {
            exit(0);
        }
        delete request;
    }
}

int main()
{
	int serverSocket;	//file descriptors
    startServer(serverSocket);
  
    // listening on the socket
    if (listen(serverSocket, 10) != 0)
    {
        cout << "\nListen failed";
        exit(0);
    }
    
    // clientThreads
    thread clientThread[7];
	
    // connect with any number of clients
    for (int i = 0; i < 7; i++)
        clientThread[i] = thread(sockListenClient, serverSocket);
	
    //waiting for closing connections
    for (int i = 0; i < 7; i++)
        clientThread[i].join();

	//closing the socket connection
	close(serverSocket);

	return 0;
	
}