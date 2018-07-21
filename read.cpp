#include "decodedBufferPlayback.hpp"
#include "ffmpegReader.hpp"

extern "C" {
    #include <stdio.h>
}

int driver = -1;
ao_device* device = NULL;

void init_playback(int bits, int channels, int sample_rate)
{
    ao_sample_format sample_format;
    sample_format.channels = channels;
    sample_format.rate = sample_rate;
    sample_format.bits = bits;
    sample_format.byte_format = AO_FMT_NATIVE;
    sample_format.matrix = 0;

    // To initalize libao for playback
    device = ao_open_live(driver, &sample_format, NULL);
}

void play_playback(char * buffer, int bufferSize)
{
    // Send the buffer contents to the audio device
    ao_play(device, buffer, bufferSize);
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


    ao_initialize(); 
    driver = ao_default_driver_id();
 
    av_register_all();

    audio_decode_example(input_filename, &init_playback, &play_playback); 
/*
    FFmpegFile file(input_filename);
    unsigned char buffer[2400];

    int frames;
    file.info(frames);
    for (int i = 1; i <= frames; i++)
    {
        file.decode(buffer, i, &init_playback, &play_playback);
    }
*/
    ao_shutdown();
  
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}