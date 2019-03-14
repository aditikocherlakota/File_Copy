#include "sharedfiledefs.h"
#include "ClientFile.h"
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include <stdbool.h>

using namespace std;
using namespace C150NETWORK;

void setUpDebugLogging(const char *logname, int argc, char *argv[]);
void requestCheck(C150DgmSocket *sock, const char *filename);
vector<string> getFiles();

void run(char *argv[]) {
    int readlen;
    vector<string>::iterator it;
    c150debug->printf(C150APPLICATION, "Creating C150DgmSocket");

    int fileNastiness = atoi(argv[3]);
    int networkNastiness = atoi(argv[2]);

    C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);

    sock->setServerName(argv[1]);
    sock->turnOnTimeouts(3000);

    server_packet serverResponse;

    vector<string> files = getFiles();

    for (it = files.begin(); it != files.end(); it++) {
        ClientFile file(*it, fileNastiness);
        cerr << "BEGIN SENDING FILE: " << *it << endl;
        while (true) {
            file.sendData(sock);
            readlen = sock->read((char*) &serverResponse, sizeof(serverResponse));
            // cerr << "I'm reading the server response packet, and it has a filename of: " << serverResponse.filename << endl; 
            if (file.shouldContinue()) {
                // cerr << "INSIDE SHOULD CONTINUE" << endl;
                if (sock->timedout() || readlen <= 0) {
                    file.timeOut();
                } else {
                    file.receiveData(serverResponse);
                }
            } else {
                cerr << "ABORTING" << endl;
                break;
            }
            if (file.isDone()) {
                cerr << "FILE IS DONE" << endl;
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    setUpDebugLogging("fileclientdebug.txt", argc, argv);

    if (argc != 5) {
        fprintf(stderr, "Correct syntxt is: %s <servername> <networknastiness> <filenastiness> <srcdir>\n", argv[0]);
        exit(1);
    }

    if (!areDigits(argv[2], argv[3])) {
        fprintf(stderr, "Nastiness is not numeric\n");
        exit(4);
    }

    chdir(argv[4]);

    try {
        run(argv);
    }
    catch (C150NetworkException e) {
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }
    return 0;
}

vector<string> getFiles() {
    DIR *SRC = opendir(".");
    struct dirent *sourceFile;
    struct stat statbuf;
    if (SRC == NULL) {
        fprintf(stderr, "Error opening source directory\n");
        exit(8);
    }

    vector<string> files;
    while ((sourceFile = readdir(SRC)) != NULL) {
        lstat(sourceFile->d_name, &statbuf);
        if (!S_ISREG(statbuf.st_mode) ||
                (strcmp(sourceFile->d_name, ".") == 0) ||
                (strcmp(sourceFile->d_name, "..") == 0 )) {
            continue;
        }

        files.push_back(sourceFile->d_name);
    }
    closedir(SRC);
    return files;
}

void setUpDebugLogging(const char *logname, int argc, char *argv[]) {

    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);

    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp();

    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC |
                             C150NETWORKDELIVERY);
}
