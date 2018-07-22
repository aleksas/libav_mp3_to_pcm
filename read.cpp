#include "decodedBufferPlayback.hpp"
#include "ffmpegReader.hpp"

extern "C" {
    #include <stdio.h>
}

typedef struct OutputHandle_ {
    int driver;
    ao_device* device;
} OutputHandle;

void die(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void init(int bits, int channels, int sample_rate, void * data)
{
    OutputHandle * pHandle = (OutputHandle *) data;
    if (!data) die("data null pointer passed to init.");

    if (!pHandle->device)
    {
        ao_sample_format sample_format;
        sample_format.channels = channels;
        sample_format.rate = sample_rate;
        sample_format.bits = bits;
        sample_format.byte_format = AO_FMT_NATIVE;
        sample_format.matrix = 0;

        // To initalize libao for playback
        pHandle->device = ao_open_live(pHandle->driver, &sample_format, NULL);
        if (!pHandle->device) die("Could not initialize output device");
    }
}

void play(char * buffer, int bufferSize, void * data)
{
    OutputHandle * pHandle = (OutputHandle *) data;
    if (!data) die("data null pointer passed to play.");

    // Send the buffer contents to the audio device
    ao_play(pHandle->device, buffer, bufferSize);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        die("Please provide the file path as the first argument");
    }
 
    const char* input_filename = argv[1];

    OutputHandle handle = {0};

    ao_initialize(); 
    handle.driver = ao_default_driver_id();
 
    av_register_all();

    int scenario = 3;

    switch (scenario){
        case 1:
            audio_decode_example(input_filename, &init, &play, &handle); 
            break;
        case 2:
            {
                FFmpegFile file(input_filename);
                int64_t frames, samples;
                file.info(frames, samples);

                int frame = 2700;
                while (file.decode(frame++, &init, &play, &handle))
                {}
            }
            break;
        case 3:
            {
                FFmpegFile file(input_filename);

                int64_t frames, samples;
                file.info(frames, samples);
                file.decodeSamples(3200000, samples - 1, &init, &play, &handle);
            }
            break;
    }
    

    ao_shutdown();
  
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}