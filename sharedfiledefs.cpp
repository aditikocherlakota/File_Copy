#include "sharedfiledefs.h"

using namespace std;

void compute_hash(const char* filename, unsigned char hash[]) {
    ifstream *t = new ifstream(filename);
    stringstream *buffer = new stringstream;

    *buffer << t->rdbuf();
    SHA1((const unsigned char *)buffer->str().c_str(),
         (buffer->str()).length(), hash);
    delete t;
    delete buffer;
}

void printHash(unsigned char hash[])
{
    for (int i = 0; i < HASH_LENGTH; i++)
    {
        printf("%x", hash[i]);
    }
    printf("\n");
}

bool areDigits(char* networkNastiness, char* fileNastiness) {
    return (strspn(networkNastiness, "0123456789") == strlen(networkNastiness))
           && (strspn(fileNastiness, "0123456789") == strlen(fileNastiness));
}