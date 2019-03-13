#ifndef SERVERFILE_H
#define SERVERFILE_H
#include "sharedfiledefs.h"

class ServerFile {
    private:

        int receivedPacketNum;
        int fileNastiness;
        int sessionNumber;

        bool done;
        NASTYFILE* file;

        void setup();
        server_packet createPacket(Type type);
        void receiveFilePacket(client_packet);
        void performCheck(C150DgmSocket *sock);
        void checkAcknowledged(C150DgmSocket *sock);
        void checkFailed(C150DgmSocket *sock);
        bool isValidPacketType(client_packet packet);
        string getTempFilename();

    public:
        
        string filename;

        ServerFile(string filename, int fileNastiness);
        void receiveData(client_packet packet, C150DgmSocket *sock);
        void sendAcks(C150DgmSocket *sock);
        bool isDone();
};

#endif