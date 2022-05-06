#include <bits/stdc++.h>
#include "connection.cpp"
#include "fileio.cpp"
using namespace std;

#define F1 "TestFile1"
#define F2 "TestFile2"
#define F3 "TestFile3"

static int hasRequested = false;

char* requestTime = new char[MAX];
static int clientId;
static int server1, server2, server3;
char *charClientId;
static bool commitReply[3] = {false};
static bool crExec[3] = {false};
static atomic<int> lampClock(0);
//map for clientid vs socket descriptors
static unordered_map<int, int> mp;

typedef struct defReplyData
{
    int timeStamp;
    int clientFd;
} defReplyData;

typedef struct replyData
{
    bool replySent = false;
    int timeStamp = 0;
    int clientFd;
} replyData;

struct compareTS
{
    bool operator()(defReplyData const &p1, defReplyData const &p2)
    {
        return p1.timeStamp > p2.timeStamp;
    }
};

//key is fileno
static map<int, replyData> Reply;

//key - {socketfd, fineno}
static map<pair<int, int>, bool> receivedReply;

static unordered_map<int, bool> failed;

static priority_queue<defReplyData, vector<defReplyData>, compareTS> deferredreply[3];

static map<pair<int, int>, bool> yieldSent;

const vector<string> ips{"10.176.69.33", "10.176.69.34", "10.176.69.35", "10.176.69.36", "10.176.69.37"};

//function to increment lamports clock
void incClock(int compTime)
{
    int temp = lampClock;
    lampClock = max(temp, compTime) + 1;
    // cout << "\nLamports clock value: " << lampClock << endl;
}

//fucntion to get fileno
int getFileNo(char fileName[])
{
    if (!strcmp(fileName, F1))
        return 1;
    else if (!strcmp(fileName, F2))
        return 2;
    return 3;
}

void getFiles(int clientSocket)
{
    cout << "\nFile list:" << endl;
    char *buffer = new char[MAX];
    if (send(clientSocket, ENQUIRY, MAX, 0) == -1)
    {
        cout << "\nError reading data from server." << endl;
        close(clientSocket);
        exit(0);
    }
    while (read(clientSocket, buffer, MAX) != -1)
    {
        if (!strcmp(buffer, DONE))
        {
            break;
        }
        cout << buffer << endl;
    }
    delete buffer;
}

void sendRequest(int id, char filename[])
{
    if (id == 0)
        id = 5;
    char *response = new char[MAX];

    incClock(lampClock);
    int *temp = new int;
    *temp = lampClock;

    // cout << "\nDebug Here to send request to client id: " << id << " on socket fd: " << mp[id] << endl;
    
    //adding sleep here if client id is 2. This is to check yield messages.
    if(clientId == 2 and id == 4)
        sleep(1);

    if (send(mp[id], REQUEST, MAX, 0) != -1)
    {
        if (send(mp[id], filename, MAX, 0) != -1)
        {
            if (send(mp[id], temp, sizeof(temp), 0) != -1)
            {
                hasRequested = true;
                cout << "\nSent request to client: " << id << " for file: " << filename << endl;
            }
        }
    }
}

void sendRelease(int tempFd, char filename[MAX])
{
    incClock(lampClock);
    if (send(tempFd, RELEASE, MAX, 0) != -1)
    {
        if (send(tempFd, filename, MAX, 0) != -1)
        {
            int fno = getFileNo(filename);
            receivedReply[{tempFd, fno}] = false;
            cout << "\nSent release to clientFd: " << tempFd << endl;
        }
    }
    int fileno = getFileNo(filename);
    if (deferredreply[fileno].size() > 0)
    {
        replyData &checkReplyData = Reply[fileno];
        defReplyData dr = deferredreply[fileno].top();
        deferredreply[fileno].pop();

        //send reply to the request on top og queue if not empty
        if (send(dr.clientFd, REPLY, MAX, 0) > 0)
        {
            if (send(dr.clientFd, filename, MAX, 0) > 0)
            {
                checkReplyData.replySent = true;
                checkReplyData.clientFd = dr.clientFd;
                checkReplyData.timeStamp = dr.timeStamp;
                cout << "\nSent reply to client for entering critical section." << endl;
            }
        }
    }
}

void serverRequest(int id, char fileName[MAX])
{
    int socketFd;
    if(id == 0)
        socketFd = server1;
    else if(id == 1)
        socketFd = server2;
    else if(id == 2)
        socketFd = server3;

    char response[MAX];

    if(send(socketFd, COMREQ, MAX, 0) != -1)
    {
        if(send(socketFd, fileName, MAX, 0) != -1)
        {
            cout<<"\nSent commit request to server.";
            if(read(socketFd, response, MAX) != -1)
            {
                if(!strcmp(response, AGREED))
                {
                    commitReply[id] = true;
                }
                else if(!strcmp(response, ABORT))
                {
                    commitReply[id] = false;
                }
            }
        }
    }
}

void serverReply(int id, bool decision, char fileName[MAX])
{
    int socketFd;
    if(id == 0)
        socketFd = server1;
    else if(id == 1)
        socketFd = server2;
    else if(id == 2)
        socketFd = server3;
    if(decision)
    {
        if(send(socketFd, COMMIT, MAX, 0) != -1)
        {
            cout<<"\nSent commit to server."<<endl;            
        }
    }
    else
    {
        if(send(socketFd, FINALABORT, MAX, 0) != -1)
        {
            cout<<"\nSent abort to server."<<endl;
        }
    }
}

void writeFile()
{
    char *filename = new char[MAX];
    char *response = new char[MAX];

    cout << "\nEnter File Name to wrtie:" << endl;
    cin >> filename;
    int fileNo = getFileNo(filename);


    
    //send request to quorom for replies.
    thread sendRequest1(sendRequest, (clientId + 1) % 5, filename);
    thread sendRequest2(sendRequest, (clientId + 2) % 5, filename);

    if(Reply[fileNo].replySent)
    {
        if(Reply[fileNo].timeStamp < lampClock)
        {
            // cout<<"\nSetting falied to true since we have a higher priority request."<<endl;
            failed[fileNo] = true;
        }
    }
    sendRequest1.join();
    sendRequest2.join();

    //loop for waiting until both replies are obtained
    int temp1 = (clientId + 1) % 5 ? (clientId + 1) % 5 : 5;
    int temp2 = (clientId + 2) % 5 ? (clientId + 2) % 5 : 5;
    while (true)
    {
        if (receivedReply[{mp[temp1], fileNo}] and receivedReply[{mp[temp2], fileNo}] and !Reply[fileNo].replySent)
        {
            crExec[fileNo] = true;
            cout << "\nCool received required replies from quorom.\nStarting 2 phase commit protocol." << endl;
            //ask servers to write. Need to implement 2 phase here.
            thread serverReq[3];
            
            for(int i=0; i<3; i++)
                serverReq[i] = thread(serverRequest, i, filename);
            
            for(int i=0; i<3; i++)
                serverReq[i].join();
            
            thread serverRep[3];

            if(commitReply[0] && commitReply[0] && commitReply[0])
            {
                cout<<"\nAll servers ready to commit. Sending commit message."<<endl;
                for(int i=0; i<3; i++)
                    serverRep[i] = thread(serverReply , i, true, filename);
                tempWrite(clientId, requestTime, filename);
            }
            else
            {
                cout<<"\nSome server sent abort for 2 phase commit. Sending abort message to remaining servers."<<endl;
                for(int i=0; i<3; i++)
                    serverRep[i] = thread(serverReply , i, false, filename);
            }
            for(int i=0; i<3; i++)
                serverRep[i].join();
            break;
        }
    }

    //once done send release to the servers.

    thread sendRelease1(sendRelease, mp[temp1], filename);
    thread sendRelease2(sendRelease, mp[temp2], filename);

    sendRelease1.join();
    sendRelease2.join();

    hasRequested = false;
    crExec[fileNo] = false;
}

void mainFunc(int clientSocket)
{
    int choice;
    while (true)
    {
        failed[1] = true;
        cout << "\nPrinting all ids: " << endl;
        for (auto &i : mp)
            cout << i.first << " " << i.second << endl;
        cout << "\nChoose\n1 Get files\n2 Write to a file\n3 Exit" << endl;
        cin.sync();
        cin >> choice;
        switch (choice)
        {
        case 1:
            getFiles(clientSocket);
            break;

        case 2:
        {
            time_t now = time(0);
            // convert now to string form
            requestTime = ctime(&now);
            writeFile();
            break;
        }
        case 3:
            send(clientSocket, DONE, MAX, 0);
            break;
        }
        if (choice == 3)
            return;
    }
}

void initQuorom(vector<string> &quorom)
{
    //cout<<"\nInitialising quorom"<<endl;

    for (int i = 1; i <= 5; i++)
        mp[i] = -1;
    for (int i = 0; i < 2; i++)
    {
        int index = (clientId + i) % 5;
        quorom.push_back(ips[index]);
        cout << quorom[i] << endl;
    }
}

//function to perform maekawas. connecting to other clients in quorom.
void serverListen(int socketFd, int id)
{
    while (true)
    {
        char *request = new char[MAX];
        char fileName[MAX];
        int *timeStamp = new int;

        if (read(socketFd, request, MAX) == -1)
        {
            cout << "\nError in reading from client" << endl;
            close(socketFd);
            exit(0);
        }
        else
            cout << "\nRequest Type: " << request << endl;
        if (!strcmp(request, REQUEST))
        {
            cout << "\nRequest received from client for entering CS: " << socketFd << endl;
            if (read(socketFd, fileName, MAX) != -1)
            {
                if (read(socketFd, timeStamp, sizeof(timeStamp)) != -1)
                {
                    incClock(*timeStamp);
                    int fileno = getFileNo(fileName);
                    replyData &checkReplyData = Reply[fileno];

                    //this is if reply has not been sent to another server in quorom.
                    //also check if currently I am not in critical section

                    if (crExec[fileno] or hasRequested)
                    {
                        cout<<"\nAlready in critical section or has requested another client. add request to queue."<<endl;
                        if (send(socketFd, FAILED, MAX, 0) != -1)
                        {
                            if (send(socketFd, fileName, MAX, 0) != -1)
                            {
                                defReplyData dr;
                                dr.timeStamp = *timeStamp;
                                dr.clientFd = socketFd;
                                deferredreply[fileno].push(dr);
                           }
                        }
                    }
                    else if (!checkReplyData.replySent)
                    {
                        cout << "\nSending reply to client." << endl;
                        if(clientId == 5)
                            sleep(1);
                        if (send(socketFd, REPLY, MAX, 0) != -1)
                        {
                            if (send(socketFd, fileName, MAX, 0) != -1)
                            {
                                checkReplyData.replySent = true;
                                checkReplyData.clientFd = socketFd;
                                checkReplyData.timeStamp = *timeStamp;
                                cout << "\nSent reply to client for entering critical section." << endl;
                            }
                        }
                    }
                    //we need to check if reply sent has a lower priority.
                    else
                    {
                        cout << "\nDebug could not send reply to the client." << endl;
                        //this means current request is of lower priority
                        if (checkReplyData.timeStamp < *timeStamp)
                        {
                            if (send(socketFd, FAILED, MAX, 0) != -1)
                            {
                                if (send(socketFd, fileName, MAX, 0) != -1)
                                {
                                    //adding it to deferredreply queue
                                    defReplyData dr;
                                    dr.timeStamp = *timeStamp;
                                    dr.clientFd = socketFd;
                                    deferredreply[fileno].push(dr);

                                    cout << "\nSent failed message. Added client to queue for further reply." << endl;
                                }
                            }
                        }
                        //this means that current request is of higher priority.
                        else
                        {
                            //few things need to be checked before sending a reply.
                            //send a inquire message
                            defReplyData dr;

                            dr.timeStamp = *timeStamp;
                            dr.clientFd = socketFd;
                            deferredreply[fileno].push(dr);

                            int inquireSocket = checkReplyData.clientFd;
                            if (send(inquireSocket, INQUIRE, MAX, 0) != -1)
                            {
                                if (send(inquireSocket, fileName, MAX, 0) != -1)
                                {
                                    cout << "\nSent inquire message to other client." << endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (!strcmp(request, REPLY))
        {
            // cout << "\nDebubg reply part 1" << endl;
            if (read(socketFd, fileName, MAX) != -1)
            {
                int fileno = getFileNo(fileName);
                receivedReply[{socketFd, fileno}] = true;
                cout << "\nReceived reply from clientFd: " << socketFd << endl;
            }
        }
        else if (!strcmp(request, INQUIRE))
        {
            if (read(socketFd, fileName, MAX) != -1)
            {
                int fileno = getFileNo(fileName);
                
                if (!crExec[fileno])
                {
                    //not executing critical section check the previous reply sent.
                    //checking if yield has been sent and if it has received a reply from it
                    int check2 = yieldSent[{socketFd, fileno}] and !receivedReply[{socketFd, fileno}];
                    
                    if (failed[fileno] or check2)
                    {
                        if (send(socketFd, YIELD, MAX, 0) > 0)
                        {
                            if (send(socketFd, fileName, MAX, 0) > 0)
                            {
                                //set reply to false since giving reply back
                                receivedReply[{socketFd, fileno}] = false;
                                yieldSent[{socketFd, fileno}] = true;
                                cout << "\nSent yield to the client." << endl;
                            }
                        }
                    }
                }
            }
        }
        else if (!strcmp(request, FAILED))
        {
            if (read(socketFd, fileName, MAX) != -1)
            {
                cout << "\nreceived failed from other client for file: " << fileName << endl;
                int fileno = getFileNo(fileName);
                failed[fileno] = true;
            }
        }
        else if (!strcmp(request, YIELD))
        {
            if (read(socketFd, fileName, MAX) != -1)
            {
                int fileno = getFileNo(fileName);
                defReplyData dr = deferredreply[fileno].top();
                deferredreply[fileno].pop();

                //insert old request at correct position in queue
                replyData tempReplyData = Reply[fileno];
                defReplyData tempdr;

                tempdr.timeStamp = tempReplyData.timeStamp;
                tempdr.clientFd = tempReplyData.clientFd;

                deferredreply[fileno].push(tempdr);

                if (send(dr.clientFd, REPLY, MAX, 0) > 0)
                {
                    if (send(dr.clientFd, fileName, MAX, 0) > 0)
                    {
                        replyData &checkReplyData = Reply[fileno];

                        checkReplyData.replySent = true;
                        checkReplyData.clientFd = socketFd;
                        checkReplyData.timeStamp = *timeStamp;

                        cout << "\nSent reply on yield to higher priority request." << endl;
                    }
                }
            }
        }
        else if (!strcmp(request, RELEASE))
        {
            //received release from other client
            if (read(socketFd, fileName, MAX) != -1)
            {
                int fileno = getFileNo(fileName);
                failed[fileno] = false;
                replyData &checkReplyData = Reply[fileno];
                checkReplyData.replySent = false;
                checkReplyData.clientFd = -1;
                checkReplyData.timeStamp = -1;

                cout << "\nReceived release from client for file: " << fileno << endl;

                if (deferredreply[fileno].size() > 0)
                {
                    defReplyData dr = deferredreply[fileno].top();
                    deferredreply[fileno].pop();

                    //send reply to the request on top og queue if not empty
                    if (send(dr.clientFd, REPLY, MAX, 0) > 0)
                    {
                        if (send(dr.clientFd, fileName, MAX, 0) > 0)
                        {
                            checkReplyData.replySent = true;
                            checkReplyData.clientFd = dr.clientFd;
                            checkReplyData.timeStamp = *timeStamp;
                            cout << "\nSent reply to client for entering critical section." << endl;
                        }
                    }
                }
            }
        }
        else if (!strcmp(request, "getId"))
        {
            if (send(socketFd, charClientId, MAX, 0) > 0)
                cout << "\nSent client id to client: " << socketFd << endl;
            else
                cout << "\nError in write." << endl;
        }

        delete request;
    }
}

void acceptClient(int serverSocket)
{
    thread startListening[2];
    int i = 0;
    // listening on the socket
    while (true)
    {
        struct sockaddr_in servaddr;
        socklen_t len = sizeof(servaddr);
        char tempid[MAX];
        int tempFd = accept(serverSocket, (SA *)&servaddr, &len);
        int tempIdint;

        if (send(tempFd, "getId", MAX, 0) != -1)
        {
            int check = read(tempFd, tempid, MAX);
            if (check > 0)
            {
                // cout << "\nDebug: Received client id :" << tempid << endl;
                tempIdint = stol(tempid);
                mp[tempIdint] = tempFd;
                startListening[i] = thread(serverListen, tempFd, tempIdint);
                i++;
            }
            else
                cout << "\nDebug Check: " << check << endl;
        }
        //cout<<"\nConnection established with client id: "<<tempIdint<<endl;
    }
}

//clientid port ip
int main(int argc, char **argv)
{
    clientId = stoi(argv[1]);
    charClientId = argv[1];
    vector<string> quorom;
    initQuorom(quorom);

    int clientSocket, serverSocket;
    thread startServerListen;

    //conenct to server
    connServer(server1, "10.176.69.32", 11111);
    connServer(server2, "10.176.69.38", 11111);
    connServer(server3, "10.176.69.39", 11111);

    //start client as a server.
    startServer(serverSocket);
    thread acceptClientThread(acceptClient, serverSocket);
    int ret = listen(serverSocket, 10);
    if (ret)
    {
        cout << "\nListen failed";
        exit(0);
    }
    else
    {
        //cout << "\nListening on socket for clients...." << endl;
    }

    sleep(5);

    char *tempcid = argv[1];
    //try connecting to quorom if possible
    int temp1 = (clientId + 1) % 5 ? (clientId + 1) % 5 : 5;
    int temp2 = (clientId + 2) % 5 ? (clientId + 2) % 5 : 5;
    connServer(mp[temp1], quorom[0].c_str(), stoi(argv[2]));
    connServer(mp[temp2], quorom[1].c_str(), stoi(argv[2]));
    thread startListen1;
    thread startListen2;
    if (mp[temp1] != -1 and mp[temp2] != -1)
    {
        cout << "\nConnections established with the quoroms" << endl;
        // call listen function here
        startListen1 = thread(serverListen, mp[temp1], temp1);
        startListen2 = thread(serverListen, mp[temp2], temp2);
    }

    //starting thread for the main function after connecting to the server.
    thread clientThread(mainFunc, server1);
    clientThread.join();

    //closing the connection to server
    close(clientSocket);

    return 0;
}
