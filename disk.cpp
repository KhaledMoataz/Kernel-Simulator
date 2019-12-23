#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include<bits/stdc++.h>
#include <fstream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

using namespace std;
int timer = 0;
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
    cout << "Disk: time" << endl;
    timer++;
}

void statusHandler(int signum) {
    cout << "Disk: Kernel requests status" << endl;
    diskStatusMessageBuffer message;
    long numberEmptySlots = 0;
    for (int i = 0; i < 10; i++) {
        if (arr[i] == "")
            numberEmptySlots++;
        message.empty[i] = (arr[i] == "");
    }
    if (numberEmptySlots == 10)
        message.mType = 2;
    else
        message.mType = 1;
    int send_val = msgsnd(diskToKernelQueueID, &message, sizeof(message.empty), !IPC_NOWAIT);
    if (send_val == -1)
        perror("Error in send");
}

int main(int argc, char **argv) {
    cout << "Hello: Disk" << endl;
//    signal(SIGUSR2, timeHandler);
//    signal(SIGUSR1, statusHandler);
//    istringstream(argv[0]) >> kernelToDiskQueueID;
//    istringstream(argv[1]) >> diskToKernelQueueID;
//    while (1) {
//        struct msgbuff message;
//        int rec_val = msgrcv(kernelToDiskQueueID, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
//        if (rec_val == -1)
//            perror("Disk: Error in receive");
//        else {
//            cout << "Disk received message!";
//            string temp = message.mtext;
//            int wait = -1;
//            if (temp[0] == 'A') {
//                wait = 3;
//                temp.erase(temp.begin(), temp.begin() + 1);
//                for (int i = 0; i < 10; i++) {
//                    if (arr[i] == "")
//                        arr[i] = temp;
//                }
//            } else {
//                wait = 1;
//                temp.erase(temp.begin(), temp.begin() + 1);
//                int x = stoi(temp);
//                for (int i = 0; i < 10; i++) {
//                    if (i == x)
//                        arr[i] = "";
//                }
//            }
//            if (wait != -1)
//                sleep(wait);
//            else cout << "Error in wait" << endl;
//
//        }
//    }
    return 0;
}

