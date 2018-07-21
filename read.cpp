#include "decodedBufferPlayback.hpp"

extern "C" {
    #include <stdio.h>
}

void die(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        die("Please provide the file path as the first argument");
    }
 
    const char* input_filename = argv[1];
 
    audio_decode_example(input_filename); 
  
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}