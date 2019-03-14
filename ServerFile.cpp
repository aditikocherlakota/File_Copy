#include "ServerFile.h"

ServerFile::ServerFile(string filename, int fileNastiness) {
    this->filename = filename;
    this->fileNastiness = fileNastiness;
    sessionNumber = 0;
    setup();
}

void ServerFile::changeState(State state) {
    this->state = state;
}

server_packet ServerFile::createPacket(Type type) {
    cerr << "creating server packet of type: " << type << endl;
    server_packet response;
    response.type = type;
    response.sessionNumber = sessionNumber;
    memcpy(&response.filename, filename.c_str(), NAME_LENGTH);
    return response;
} 

void ServerFile::setup() {
    cerr << "IN SETUP" << endl;
    done = false;
    receivedPacketNum = -1;
    changeState(INITIAL);
    file = new C150NastyFile(fileNastiness);
    void *fopenretval = file->fopen(getTempFilename().c_str(), "wb");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << filename << endl;
        exit(12);
    }
}

void ServerFile::sendAcks(C150DgmSocket *sock) {
    server_packet response = createPacket(FILE_ACK);
    sprintf((char *) &response.hash, "%d", receivedPacketNum);
    sock->write((char*) &response, sizeof(response));
}


void ServerFile::receiveFilePacket(client_packet packet) {
    if (packet.num == receivedPacketNum + 1) {
        file->fwrite(packet.datagram, 1, packet.size);
        receivedPacketNum = packet.num;
    }
}

void ServerFile::performCheck(C150DgmSocket *sock) {
    cerr << "HITTING PERFORM CHECK" << endl;
    file->fclose();
    delete file;

    unsigned char hash[HASH_LENGTH];
    server_packet response = createPacket(E2E_CONF);
    compute_hash(getTempFilename().c_str(), hash);
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
    setup();
}

bool ServerFile::isDone() {
    return done;
}

string ServerFile::getTempFilename() {
    return filename + string(".TMP");
}

bool ServerFile::isValidPacketType(client_packet packet) {
    return packet.sessionNumber == sessionNumber;
}

void ServerFile::receiveData(client_packet packet, C150DgmSocket *sock) {
    cerr << "CURRENT STATE IS: " << state << endl;
    cerr << "I'm the server receiving packet with filename: " << packet.filename << endl;
    cerr << "and a type of: " << packet.type << endl;
    if (!isValidPacketType(packet)) {
        cerr << "DROPPING PACKET" << endl;
        return;
    }
    sessionNumber = packet.sessionNumber;
    if (packet.type == FILE_PACKET && state == INITIAL) {
        receiveFilePacket(packet);
    } else if (packet.type == E2E_CHECK && state == INITIAL) {
        *GRADING << "File: " << packet.filename << " received, beginning end-to-end check" << endl;
        performCheck(sock);
        changeState(WAITING_FOR_E2E_RESULT);
    } else if (packet.type == E2E_ACK && state == WAITING_FOR_E2E_RESULT) {
        *GRADING << "File: " << packet.filename << " end-to-end check succeeded" << endl;
        checkAcknowledged(sock);
        changeState(DONE);
    } else if (packet.type == E2E_FAIL && state == WAITING_FOR_E2E_RESULT) {
        *GRADING << "File: " << packet.filename << " end-to-end check failed" << endl;
        checkFailed(sock);
    } else {
        return;
    }
}