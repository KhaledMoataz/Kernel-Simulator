#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

struct diskStatusMessageBuffer {
    long mType;                 //1: an empty slot exist, 2: all slots are full....0: invalid
    bool empty[10];             //0 for full slot, 1 for an empty one
};

void handler(int s) {
}

int main(int argc, char **argv) {
    signal(SIGUSR2, handler);
    key_t kernelToDiskQueueID, diskToKernelQueueID;
    istringstream(argv[0]) >> kernelToDiskQueueID;
    istringstream(argv[1]) >> diskToKernelQueueID;
    sleep(10);
    while(true) {}
    return 0;
}

