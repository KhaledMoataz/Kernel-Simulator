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

bool compare(const tuple<int, int, string> &a, const tuple<int, int, string> &b) {
    return (get<0>(a) > get<0>(b));
}


int main(int argc, char **argv) {
    signal(SIGUSR2, timeHandler);
    int processNumber;
    key_t processKernelQueueID;
    istringstream(argv[0]) >> processNumber;
    istringstream(argv[1]) >> processKernelQueueID;
    fstream file;
    file.open("file.txt");
    vector <string> lines;
    string x, y;
    int k = 0;
    while (!file.eof()) {
        if (k == 2) {
            getline(file, y);
            y.erase(y.begin());
            lines.push_back(y);
            k = 0;
        } else {
            file >> x;
            lines.push_back(x);
            k++;
        }
    }
    lines.erase(lines.end());
    int size = lines.size() / 3;
    int j = 0;
    vector <tuple<int, int, string>> messages; //time operation message

    for (int i = 0; i < size; i++) {
        int time;
        istringstream(lines[j]) >> time;
        j++;
        int type;
        string msg;
        transform(lines[j].begin(), lines[j].end(), lines[j].begin(), ::tolower);
        if (lines[j] == "add") {
            j++;
            type = 1;
            msg = "A " + lines[j];
            j++;
        } else if (lines[j] == "del") {
            j++;
            type = 2;
            msg = "D " + lines[j];
            j++;
        }
        //else error
        messages.push_back(make_tuple(time, type, msg));
    }
    sort(messages.begin(), messages.end(), compare);
    while (!messages.empty()) {
        while (timer < get<0>(messages.back())) pause();
        do {
            struct operationMessageBuffer message;
            string msg = get<2>(messages.back());
            strncpy(message.mText, msg.c_str(), sizeof(message.mText));
            message.mType = get<1>(messages.back());
            messages.pop_back();
            int send_val = msgsnd(processKernelQueueID, &message, sizeof(message.mText), !IPC_NOWAIT);
            if (send_val == -1) {
                perror("Error in send");
            }
        } while (!messages.empty() && timer >= get<0>(messages.back()));
    }
    file.close();
    return 0;
}

