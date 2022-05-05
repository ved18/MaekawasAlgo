#include <bits/stdc++.h>
using namespace std;
#define MAX 256
#define FS1 "./FS1/"
#define FS2 "./FS2/"
#define FS3 "./FS3/"

void tempWrite(int clientId, char timeStamp[], char fileName[])
{

    this_thread::sleep_for(chrono::milliseconds(5000));
    ofstream f;

    for(int i=0; i<3; i++)
    {
        char dir[MAX];
        if(i == 0)
            memcpy(dir, FS1, sizeof(FS1));
        else if(i == 1)
            memcpy(dir, FS2, sizeof(FS2));
        else if(i == 2)
            memcpy(dir, FS3, sizeof(FS3));
        
        strcat(dir, fileName);

        f.open(dir, std::ios_base::app);

        if (f.is_open())
        {
            f << "Client ID: " << clientId << "\nWrite Time: " << timeStamp << endl;
        }
        else
        {
            cout << "\nCould not open the file" << endl;
        }

        f.close();
    }
    
}