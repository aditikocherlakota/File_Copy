#include "ClientFile.h"

ClientFile::ClientFile(string filename, int fileNastiness) {
    totalTries = 0;
    this->filename = filename;
    this->fileNastiness = fileNastiness;
    setup();
}

void ClientFile::setup() {
    changeState(INITIAL);

    *GRADING << "File: " << filename << ", beginning transmission, attempt " << totalTries << endl;

    receivedPacketNum = -1;
    finalPacketNum = -1;
    windowPacketNum = WINDOW_SIZE;

    file = new C150NastyFile(fileNastiness);
    void *fopenretval = file->fopen(filename.c_str(), "rb");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << filename << endl;
        exit(12);
    }
}

void ClientFile::changeState(State state) {
    this->state = state;
}

void ClientFile::timeOut() {
    numTimeouts++;
}

bool ClientFile::shouldContinue() {
    cerr << "TOTAL TRIES: " << totalTries << endl;
    if (numTimeouts > RETRIES_BEFORE_FAILURE) {
        *GRADING << "File: " << filename << " Aborting File Copy- Too Many Socket Timeouts" << endl;
        return false;
    }
    else if (totalTries > RETRIES_BEFORE_FAILURE) {
        *GRADING << "File: " << filename << " Aborting File Copy- Too Many Failed End-To-End Checks" << endl;
        return false;
    }
    return true;
}

bool ClientFile::isDone() {
    return state == DONE;
}

void ClientFile::sendData(C150DgmSocket *sock) {
    if (state == INITIAL) {
        sendFilePackets(sock);
    } else if (state == SENDING_E2E_CHECK) {
        send(sock, E2E_CHECK);
    } else if (state == SENDING_E2E_CONF) {
        send(sock, E2E_ACK);
    } else if (state == SENDING_E2E_FAILED) {
        // cerr << "IN SENDING_E2E_FAILED" << endl;
        send(sock, E2E_FAIL);
    }
}

void ClientFile::receiveData(server_packet serverResponse) {
    numTimeouts = 0;
    if (serverResponse.sessionNumber != totalTries || strcmp(filename.c_str(), serverResponse.filename) != 0) {
        return;
    }
    if (state == INITIAL && serverResponse.type == FILE_ACK) {
        int ackNum = atoi((const char *) serverResponse.hash);
        if (ackNum > receivedPacketNum) {
            receivedPacketNum = ackNum;
        }
        if (receivedPacketNum >= windowPacketNum) {
            windowPacketNum += WINDOW_SIZE;
        }
        if (receivedPacketNum == finalPacketNum && finalPacketNum != -1) {
            *GRADING << "File: " << filename << " transmission complete, waiting for end-to-end check, attempt " << totalTries << endl;
            changeState(SENDING_E2E_CHECK);
        }
    } else if (state == SENDING_E2E_CHECK && serverResponse.type == E2E_CONF) {
        // cerr << "ready to compute hash" << endl;
        unsigned char hash[HASH_LENGTH];
        compute_hash(filename.c_str(), hash);
        if (memcmp(hash, serverResponse.hash, HASH_LENGTH) == 0) {
            *GRADING << "File: " << filename << " end-to-end check succeeded, attempt " << totalTries << endl;
            changeState(SENDING_E2E_CONF);
        } else {
            *GRADING << "File: " << filename << " end-to-end check failed, attempt " << totalTries << endl;
            changeState(SENDING_E2E_FAILED);
        }
    } else if (state == SENDING_E2E_CONF && serverResponse.type == E2E_COMPLETE) {
        changeState(DONE);
        file->fclose();
        delete file;
    } else if (state == SENDING_E2E_FAILED && serverResponse.type == E2E_COMPLETE) {
        totalTries++;
        changeState(INITIAL);
        file->fclose();
        delete file;
        setup();
    }
}

void ClientFile::sendFilePackets(C150DgmSocket *sock) {
    int packetNum = receivedPacketNum + 1;

    while (packetNum <= windowPacketNum) {
        file->fseek(packetNum * DATAGRAM_LENGTH, SEEK_SET);
        client_packet packet = createPacket(FILE_PACKET);
        packet.num = packetNum;
        packet.size = (int) file->fread((char*) &packet.datagram, 1, DATAGRAM_LENGTH);
        sock->write((char*) &packet, sizeof(packet));

        if (packet.size < DATAGRAM_LENGTH) {
            finalPacketNum = packetNum;
            break;
        }
        packetNum++;
    }
}

void ClientFile::send(C150DgmSocket *sock, Type type) {
    client_packet packet = createPacket(type);
    sock->write((char*) &packet, sizeof(packet));
}

client_packet ClientFile::createPacket(Type type) {
    client_packet packet;
    packet.type = type;
    packet.sessionNumber = totalTries;
    // cerr << "IM SENDING OUT A PACKET W FILENAME " << filename.c_str() << endl;
    memcpy(&packet.filename, filename.c_str(), filename.length() + 1);
    return packet;
}