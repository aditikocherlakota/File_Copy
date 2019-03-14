#include "ServerFile.h"

ServerFile::ServerFile(string filename, int fileNastiness) {
    this->filename = filename;
    this->fileNastiness = fileNastiness;
    sessionNumber = 0;
    setup();
}

server_packet ServerFile::createPacket(Type type) {
    // cerr << "creating server packet of type: " << type << endl;
    server_packet response;
    response.type = type;
    response.sessionNumber = sessionNumber;
    memcpy(&response.filename, filename.c_str(), NAME_LENGTH);
    return response;
} 

void ServerFile::openFile() {
    void *fopenretval = file->fopen(getTempFilename().c_str(), "ab");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << filename << endl;
        exit(12);
    } 
}

void ServerFile::setup() {
    // cerr << "IN SETUP" << endl;
    done = false;
    receivedPacketNum = -1;
    file = new C150NastyFile(fileNastiness);
    openFile();
}

void ServerFile::sendAcks(C150DgmSocket *sock) {
    server_packet response = createPacket(FILE_ACK);
    sprintf((char *) &response.hash, "%d", receivedPacketNum);
    sock->write((char*) &response, sizeof(response));
}


void ServerFile::receiveFilePacket(client_packet packet) {
    cerr << "inside receive file packet" << endl;
    if (packet.num == receivedPacketNum + 1) {
        cerr << "writing to file" << endl;
        file->fwrite(packet.datagram, 1, packet.size);
        receivedPacketNum = packet.num;
    }
}

void ServerFile::performCheck(C150DgmSocket *sock) {
    // cerr << "HITTING PERFORM CHECK" << endl;
    file->fclose();

    unsigned char hash[HASH_LENGTH];
    server_packet response = createPacket(E2E_CONF);
    compute_hash(getTempFilename().c_str(), hash);
    openFile();
    memcpy(&response.hash, hash, HASH_LENGTH);
    sock->write((char*) &response, sizeof(response));
}

void ServerFile::checkAcknowledged(C150DgmSocket *sock) {
    server_packet response = createPacket(E2E_COMPLETE);
    sock->write((char*) &response, sizeof(response));
    rename(getTempFilename().c_str(), filename.c_str());
    done = true;
}

void ServerFile::checkFailed(C150DgmSocket *sock) {
    server_packet response = createPacket(E2E_COMPLETE);
    sock->write((char*) &response, sizeof(response));
    remove(getTempFilename().c_str());
    sessionNumber++;
    setup();
}

bool ServerFile::isDone() {
    return done;
}

string ServerFile::getTempFilename() {
    return filename + string(".TMP");
}

void ServerFile::receiveData(client_packet packet, C150DgmSocket *sock) {
    // cerr << "I'm the server receiving packet with filename: " << packet.filename << endl;
    // cerr << "and a type of: " << packet.type << endl;

    cerr << "CLIENT PACKET SESSION NUMBER: " << packet.sessionNumber << endl;
    cerr << "CLIENT PACKET SESSION NUMBER: " << sessionNumber << endl;

    if (packet.sessionNumber != sessionNumber) {
        return;
    }

    if (packet.type == FILE_PACKET) {
        receiveFilePacket(packet);
    } else if (packet.type == E2E_CHECK) {
        *GRADING << "File: " << packet.filename << " received, beginning end-to-end check" << endl;
        performCheck(sock);
    } else if (packet.type == E2E_ACK) {
        *GRADING << "File: " << packet.filename << " end-to-end check succeeded" << endl;
        checkAcknowledged(sock);
    } else if (packet.type == E2E_FAIL) {
        *GRADING << "File: " << packet.filename << " end-to-end check failed" << endl;
        checkFailed(sock);
    } else {
        fprintf(stderr, "Unknown packet type\n");
        exit(1);
    }
}