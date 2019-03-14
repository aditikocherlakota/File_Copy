#include "sharedfiledefs.h"
#include "ServerFile.h"
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <map>

using namespace C150NETWORK;

void setUpDebugLogging(const char *logname, int argc, char *argv[]);

void run(char *argv[]) {
    map<string, ServerFile*> files;
    ssize_t readlen;
    int fileNastiness = atoi(argv[2]);
    int networkNastiness = atoi(argv[1]);
    c150debug->printf(C150APPLICATION, "Creating C150NastyDgmSocket(nastiness=%d)",
                      networkNastiness);

    C150DgmSocket *sock = new C150NastyDgmSocket(networkNastiness);
    sock->turnOnTimeouts(1000);

    while (true) {
        client_packet packet;
        readlen = sock->read((char *)&packet, sizeof(packet));

        if (sock->timedout() || readlen <= 0) {
            for (std::map<string,ServerFile*>::iterator it=files.begin(); it!=files.end(); ++it){
                ServerFile* file = it->second;
                if (!file->isDone()) {
                    file->sendAcks(sock);
                }
            }
        } else {
            if (files.count(packet.filename) < 1) {
                ServerFile* file = new ServerFile(string(packet.filename), fileNastiness); 
                files.insert({string(packet.filename), file});
                cerr << "making new server file: " << string(packet.filename) << endl;
                *GRADING << "File: " << packet.filename << " starting to receive file" << endl;
            }
            files.find(packet.filename)->second->receiveData(packet, sock);
        }   
    }
}

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);

    setUpDebugLogging("fileserverdebug.txt", argc, argv);
    c150debug->setIndent("    ");

    if (argc != 4)  {
        fprintf(stderr, "Correct syntxt is: fileserver <networknastiness> <filenastiness> TARGET\n");
        exit(1);
    }

    if (!areDigits(argv[1], argv[2])) {
        fprintf(stderr, "Nastiness is not numeric\n");
        exit(4);
    }

    chdir(argv[3]);

    try {
        run(argv);
    }
    catch (C150NetworkException e) {
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        cerr << argv[0] << ": caught C150NetworkException: " << e.formattedExplanation() << endl;
    }

    return 4;
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

