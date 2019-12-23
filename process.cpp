#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <vector>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
using namespace std;

int timer = 0;

void timeHandler(int signum) {
    timer++;
}

struct operationMessageBuffer {
    long mType;                 //1: Add, 2: DEL
    char mText[64];             //In case of DEL, slotNumber = mText[0]
};


int main(int argc, char **argv) {

    signal(SIGUSR2, timeHandler);
    int processNumber;
    key_t processKernelQueueID;
    istringstream(argv[1]) >> processNumber;
    istringstream(argv[2]) >> processKernelQueueID;
    fstream file;
    file.open("file.txt");
    vector<string> lines;
    string x,y;
    int k=0;
    while(!file.eof())
    {
        if(k==2)
        {
            getline(file, y);
            y.erase(y.begin());
            lines.push_back(y);
            k=0;
        }
        else
        {
            file>>x;
            lines.push_back(x);
            k++;
        }
    }
    lines.erase(lines.end());
    int size = lines.size()/3;
    int j=0;
    vector<tuple<int,int,string>>messeges; //time operation message
    
    for(int i=0; i < size; i++)
    {
        int time; 
        istringstream(lines[j]) >> time;
        j++;
        int type;
        string msg;
        transform(lines[j].begin(),lines[j].end(),lines[j].begin(),::tolower);
        if(lines[j]=="add")
        {
            j++;
            type = 1;
            msg ="A "+lines[j];
            j++;
        }
        else if (lines[j]=="del")
        {
            j++;
            type = 0;
            msg ="D "+lines[j];
            j++;
        }
        //else error
        messeges.push_back(make_tuple(time,type,msg));
    }   
    int i=0;
    while(!messeges.empty())
    {
        if(get<0>(messeges[i]) <= timer){
            struct operationMessageBuffer message;
            string msg = get<2>(messeges[i]);
            strncpy(message.mText, msg.c_str(), sizeof(message.mText) - 1);
            message.mType = get<1>(messeges[i]);
            messeges.erase(messeges.begin() + i);
            int send_val = msgsnd(processKernelQueueID, &message, sizeof(message.mText), !IPC_NOWAIT);
            if (send_val == -1) {
                perror("Error in send");
            }
        }
        i+=1%messeges.size();   
    }
    file.close();
    return 0;
}

