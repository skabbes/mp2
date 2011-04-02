#include "mp2_sha1-c/sha1.h"
#include "mp2_sha1-c/sha1.c"
#include <string>
using namespace std;

int SHA1(string filename, int m){

    SHA1Context sha;
    SHA1Reset(&sha);
    SHA1Input(&sha, (unsigned char *)filename.c_str(), filename.length());


    if (!SHA1Result(&sha))
    {
        return -1;
    }
    else
    {
        return sha.Message_Digest[4] % ( 1 << m );
    }
}
