#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <fstream>

using namespace std;

struct operationMessageBuffer {
    long mType;                 //1: Add, 2: DEL
    char mText[64];             //In case of DEL, slotNumber = mText[0]
};

struct diskStatusMessageBuffer {
    long mType;                 //1: an empty slot exist, 2: all slots are full....0: invalid
    bool empty[10];             //0 for full slot, 1 for an empty one
};

int PROCESSES_NUM = 1;
int disk_timer = 0;
int timer = 0;
pid_t disk_pid = 0;
bool is_disk_killed;
bool nothingToDo;
ofstream log ("log.txt");
struct diskStatusMessageBuffer lastStatus;

void logDiskState(struct diskStatusMessageBuffer message) {
    string line = "At time " + to_string(timer) + ": [" + (message.empty[0] ? "true" : "false");
    int size = sizeof(message.empty) / sizeof(message.empty[0]);
    for (int i = 1; i < size; ++i) {
        line += (message.empty[i] ? ", true" : ", false");
    }
    line += "]\n";
    log << line;
}

void cancel() {
    nothingToDo = true;
    if (!is_disk_killed) {
        kill(disk_pid, SIGKILL);
        pause();
    }
    log.close();
}

void alarmHandler(int signum) {
    timer++;
    if (disk_timer > 0) disk_timer--;
    killpg(getpid(), SIGUSR2);
    alarm(1);

}

void childHandler(int signum) {

    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid == disk_pid) {
            is_disk_killed = true;
        } else if (pid > 0) {
            PROCESSES_NUM--;
        } else {
            break;
        }
    }
}

void sendOperationToDisk(key_t msgqid, struct operationMessageBuffer message) {
    string logLine = "At time " + to_string(timer) + ": disk order -> " + message.mText + "\n";
    log << logLine;
    int send_val;
    send_val = msgsnd(msgqid, &message, sizeof(message.mText), IPC_NOWAIT);
    if (send_val == -1) {
        perror("Error in send");
        return;
    }

    if (message.mType == 1) disk_timer = 3;
    else disk_timer = 1;

    lastStatus.mType = 0;
}

struct operationMessageBuffer readProcessMessageQueue(key_t msgqid) {
    int rec_val;
    struct operationMessageBuffer message;

    message.mType = 0;
    cout << "Receiving at " << timer << endl;
    rec_val = msgrcv(msgqid, &message, sizeof(message.mText), 0, !IPC_NOWAIT);
    if (rec_val == -1 && PROCESSES_NUM == 0) cancel();

    return message;
}

struct diskStatusMessageBuffer readStatus(key_t msgqid) {
    if (lastStatus.mType != 0) return lastStatus;
    int rec_val;
    struct diskStatusMessageBuffer message;
    kill(disk_pid, SIGUSR1);
    rec_val = msgrcv(msgqid, &message, sizeof(message.empty), 0, !IPC_NOWAIT);
    if (rec_val == -1) {
        perror("Error in receive");
        message.mType = 0;
    } else {
        message.mType = 1;
    }
    lastStatus = message;
    return lastStatus;
}

bool isValid(struct operationMessageBuffer operation, struct diskStatusMessageBuffer status) {
    if (operation.mType == 1) {
        return status.mType == 1;
    } else {
        return status.empty[operation.mText[0] - '0'];
    }
}

int main() {
    pid_t pid;
    key_t kernelToDiskQueueID, diskToKernelQueueID, processKernelQueueID;

    kernelToDiskQueueID = msgget(IPC_PRIVATE, 0644);
    diskToKernelQueueID = msgget(IPC_PRIVATE, 0644);
    processKernelQueueID = msgget(IPC_PRIVATE, 0644);
    if (diskToKernelQueueID == -1 || kernelToDiskQueueID == -1 || processKernelQueueID == -1) {
        perror("Error in create");
        exit(-1);
    }
    cout << "kernelToDiskQueueID: " << kernelToDiskQueueID << endl;
    cout << "diskToKernelQueueID: " << diskToKernelQueueID << endl;
    cout << "processKernelQueueID: " << processKernelQueueID << endl;

    pid = fork();
    if (pid == 0) {
        string arg1 = to_string(kernelToDiskQueueID);
        string arg2 = to_string(diskToKernelQueueID);
        cout << "Starting disk " << arg1 << " " << arg2 << endl;
        execl("./disk", arg1.c_str(), arg2.c_str());     //Start Disk, pass queue ID as an argument
    } else if (pid != -1) {
        disk_pid = pid;
        for (int i = 0; i < PROCESSES_NUM; ++i) {
            pid = fork();
            if (pid == 0) {
                string arg1 = to_string(i);
                string arg2 = to_string(processKernelQueueID);
                execl("./process", arg1.c_str(), arg2.c_str());
            } else if (pid == -1) {
                perror("Error in fork");
                exit(-1);
            }
        }
    } else {
        perror("Error in fork");
        exit(-1);
    }

    //Kernel remaining code
    signal(SIGALRM, alarmHandler);
    signal(SIGCHLD, childHandler);
    signal(SIGUSR2, SIG_IGN);
    alarm(1);
    struct operationMessageBuffer operationMessage;
    struct diskStatusMessageBuffer statusMessage;

    while (!nothingToDo) {
        while (disk_timer != 0) {
            cout << timer << " " << disk_timer << endl;
            pause();
        }
        operationMessage = readProcessMessageQueue(processKernelQueueID);
        if (operationMessage.mType == 0) { //No message received
            continue;
        }
        statusMessage = readStatus(diskToKernelQueueID);
        if (isValid(operationMessage, statusMessage)) {
            sendOperationToDisk(kernelToDiskQueueID, operationMessage);
        } else {
            //Delete message
        }
    }
    return 0;
}

