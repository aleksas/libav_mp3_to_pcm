#include "decodedBufferPlayback.hpp"
#include "ffmpegReader.hpp"

extern "C" {
    #include <stdio.h>
}

int driver = 0;
ao_device* device = NULL;

void init(int bits, int channels, int sample_rate, void * data)
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

void play(char * buffer, int bufferSize, void * data)
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

    // Just play all samples
    //audio_decode_example(input_filename, &init, &play, NULL); 

    /// Use seek:
    FFmpegFile file(input_filename);

    int64_t frames, samples;
    file.info(frames, samples);
    bool initPlayback = true;
    int i = 2700;
    while (file.decode(i++, initPlayback, &init, &play, NULL))
    {}

    ao_shutdown();
  
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}