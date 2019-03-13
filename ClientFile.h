#ifndef CLIENTFILE_H
#define CLIENTFILE_H
#include "sharedfiledefs.h"

#define WINDOW_SIZE 5
#define RETRIES_BEFORE_FAILURE 5

enum State{INITIAL, SENDING_E2E_CHECK, SENDING_E2E_CONF, SENDING_E2E_FAILED, DONE};

class ClientFile {

    private:

        State state;
        int fileNastiness;
        int numTimeouts;
        int receivedPacketNum;
        int totalTries;
        int finalPacketNum;
        int windowPacketNum;
        string filename;
        NASTYFILE* file;

        void changeState(State state);
        void send(C150DgmSocket *sock, Type type);
        void sendFilePackets(C150DgmSocket *sock);
        client_packet createPacket(Type type);
        void setup();

    public:
        
        ClientFile(string filename, int fileNastiness);
        bool isDone();
        void timeOut();
        bool shouldContinue();
        void sendData(C150DgmSocket *sock);
        void receiveData(server_packet serverResponse);

};

#endif