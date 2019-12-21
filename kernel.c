#define PROCESSES_NUM 3
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct operationMessageBuffer
{
    long mtype;                 //1: Add, 2: DEL
    char mtext[64];             //In case of DEL, slotNumber = mtext[0]
};

struct diskStatusMessageBuffer
{
    long mtype;                 //1: an empty slot exist, 2: all slots are full....0: invalid
    bool empty[10];             //0 for full slot, 1 for an empty one
};

int disk_timer = 0, time = 0;
struct diskStatusMessageBuffer lastStatus;

void sigalarmHandler(int signum){
    //Put your logic here
}

void sendOperationTodisk(key_t msgqid, struct operationMessageBuffer message)
{
    int send_val;
    send_val = msgsnd(msgqid, &message, sizeof(message.mtext), IPC_NOWAIT);
    if(send_val == -1) {perror("Errror in send"); return;}

    if (message.mtype == 1) disk_timer = 3;
    else disk_timer = 1;

    lastStatus.mtype = 0;
}

struct operationMessageBuffer readProcessMessageQueue(key_t msgqid)
{
    int rec_val;
    struct operationMessageBuffer message;

    message.mtype = 0;
    rec_val = msgrcv(msgqid, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);  
    if(rec_val == -1) perror("Error in receive");

    return message;
}

struct diskStatusMessageBuffer readStatus(key_t msgqid)
{
    //Edit your own logic
    //Check for lastStatus.mtype, if not zero..return
    int rec_val;
    struct diskStatusMessageBuffer message;

    rec_val = msgrcv(msgqid, &message, sizeof(message.empty), 0, !IPC_NOWAIT);  

    if(rec_val == -1) perror("Error in receive");

    return message;
}

bool isValid(struct operationMessageBuffer operation, struct diskStatusMessageBuffer status){
    if (operation.mtype == 1){
        return status.mtype == 1;
    } else{
        return status.empty[operation.mtext[0] - '0'];
    }
}

int main()
{
    pid_t pid;
    key_t processKernelQueueID, kernelDiskQueueID;

    processKernelQueueID = msgget(IPC_PRIVATE, 0644); 
    kernelDiskQueueID = msgget(IPC_PRIVATE, 0644); 
    if(processKernelQueueID == -1 || kernelDiskQueueID == -1)
    {	
        perror("Error in create");
        exit(-1);
    }
    printf("kernelDiskQueueID = %d\n processKernelQueueID = %d\n", kernelDiskQueueID, processKernelQueueID);

    pid = fork();
    if(pid == 0){
        execl("disk.c", kernelDiskQueueID);     //Start Disk, pass queue ID as an argument
    }
    else if(pid != -1){
        for (int i = 0; i < PROCESSES_NUM; ++i) {
            pid = fork();
            if (pid == 0) {
                execl("process.c", i, processKernelQueueID);     //Start Process, pass: number of process(to be used to open a specific file), queue ID as an argument
            } else if (pid == -1){
                perror("Error in fork");  	  	
                exit(-1);
            }
        }
    }
    else
    { 	
        perror("Error in fork");  	  	
        exit(-1);
    }

    //Kernel remaining code
    signal(SIGALRM, sigalarmHandler);
    alarm(1);
    struct operationMessageBuffer operationMessage;
    struct diskStatusMessageBuffer statusMessage;

    while (1){
        while (disk_timer != 0) sleep(5);
        operationMessage = readProcessMessageQueue(processKernelQueueID);
        if (operationMessage.mtype == 0){ //No message received
            continue;
        }
        statusMessage = readStatus(kernelDiskQueueID);
        if (isValid(operationMessage, statusMessage)){
            sendOperationTodisk(kernelDiskQueueID, operationMessage);
        } else{
            //Delete message
        }
    }
    return 0;
}

