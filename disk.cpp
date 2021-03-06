#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <bits/stdc++.h>
#include <fstream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

using namespace std;
int timer = 0;
bool nothingToDo;
key_t kernelToDiskQueueID, diskToKernelQueueID;
vector <string> arr(10, "");//10 slots each one is 64 char
struct msgbuff {
    long mtype;
    char mtext[64];

};
struct diskStatusMessageBuffer {
    long mType;                 //1: an empty slot exist, 2: all slots are full....0: invalid
    bool empty[10];             //0 for full slot, 1 for an empty one
};

void timeHandler(int signum) {
    timer++;
}

void intHandler(int signum) {
    ofstream log("disk.txt");
    for (string line : arr) {
        if (!line.empty())
            log << line << endl;
    }
    log.close();
    nothingToDo = true;
}

void statusHandler(int signum) {
    cout << "Ask about status" << endl;
    diskStatusMessageBuffer message;
    long numberEmptySlots = 0;
    for (int i = 0; i < 10; i++) {
        if (arr[i] == "")
            numberEmptySlots++;
        message.empty[i] = (arr[i] == "");
    }
    if (numberEmptySlots > 0)
        message.mType = 1;
    else
        message.mType = 2;
    int send_val = msgsnd(diskToKernelQueueID, &message, sizeof(message.empty), !IPC_NOWAIT);
    if (send_val == -1)
        perror("Error in send in disk");
}

int main(int argc, char **argv) {
    signal(SIGUSR2, timeHandler);
    signal(SIGUSR1, statusHandler);
    signal(SIGINT, intHandler);
    istringstream(argv[0]) >> kernelToDiskQueueID;
    istringstream(argv[1]) >> diskToKernelQueueID;
    while (!nothingToDo) {
        struct msgbuff message;
        int rec_val = msgrcv(kernelToDiskQueueID, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
        if (rec_val != -1) {
            cout << "Message received in disk" << endl;
            string temp = message.mtext;
            int wait = -1;
            if (temp[0] == 'A') {
                wait = 3;
                temp.erase(temp.begin(), temp.begin() + 1);
                cout << temp << endl;
                for (int i = 0; i < 10; i++) {
                    if (arr[i] == "") {
                        arr[i] = temp;
                        break;
                    }
                }
            } else {
                wait = 1;
                temp.erase(temp.begin(), temp.begin() + 1);
                cout << temp << endl;
                int x = stoi(temp);
                for (int i = 0; i < 10; i++) {
                    if (i == x) {
                        arr[i] = "";
                        break;
                    }
                }
            }

            sleep(wait);
        }
    }
    return 0;
}