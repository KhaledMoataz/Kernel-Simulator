#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
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
    istringstream(argv[0]) >> processNumber;
    istringstream(argv[1]) >> processKernelQueueID;
    cout << "Process Number: " << processNumber << ", PID: " << getpid() << endl;
    cout << "processKernelQueueID: " << processKernelQueueID << endl;
    struct operationMessageBuffer message;
    message.mType = 1;
    strncpy(message.mText, "Write this!", sizeof(message.mText) - 1);
    while (true) {
        int send_val = msgsnd(processKernelQueueID, &message, sizeof(message.mText), !IPC_NOWAIT);
        if (send_val == -1) {
            perror("Error in send");
        }
        if (timer == 5)
            break;
        pause();
    }
    return 0;
}

