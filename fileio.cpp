#include<bits/stdc++.h>
using namespace std;
#define MAX 256
#define FS1 "./FS1/"
#define FS2 "./FS2/"
#define FS3 "./FS3/"

void tempWrite(int clientId, char timeStamp[], char fileName[])
{

    this_thread::sleep_for(chrono::milliseconds(7000));
    ofstream f;
    f.open(fileName);
    
    if(f.is_open())
    {
        f<<"Client ID: "<<clientId<<"\nWrite Time: "<<timeStamp<<endl;
    }
    else
    {
        cout<<"\nCould not open the file"<<endl;
    }

    f.close();
}