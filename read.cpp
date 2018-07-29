// Silent samples after seek in mp3: https://ffmpeg.org/pipermail/libav-user/2016-September/009608.html
// Encode to m4a(AAC): ffmpeg -i Aiste.wav -c:a aac -b:a 28k -strict -2 Aiste.m4a
// https://stackoverflow.com/questions/14989397/how-to-convert-sample-rate-from-av-sample-fmt-fltp-to-av-sample-fmt-s16


typedef void (*init_playback_callback)(int bits, int channels, int sample_rate, void * callbackData);
typedef void (*play_callback)(char * buffer, int bufferSize, void * callbackData);

extern "C" {
    #include <ao/ao.h>
    #include <stdio.h>
}

#include "ffmpegReader.hpp"

typedef struct OutputHandle_ {
    int driver;
    ao_device* device;

    // Alt scenario when we save audio to mem instead of playing it
    int8_t * buffer;
} OutputHandle;

void die(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void init(int bits, int channels, int sample_rate, void * callbackData)
{
    OutputHandle * pHandle = (OutputHandle *) callbackData;
    if (!callbackData) die("data null pointer passed to init.");

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

    switch (scenario)
    {
        case 2:
            {
                FFmpegFile file(input_filename);
                
                int64_t frames, samplesPerFrame, bitsPerSample, bufferSize, bytesWritten = 0, _bytesWritten;
                int channels, sampleRate;
                file.info(frames, samplesPerFrame, bitsPerSample, channels, sampleRate);

                bufferSize = frames * samplesPerFrame * bitsPerSample / 8;

                handle.buffer = (int8_t*) realloc(handle.buffer, bufferSize);

                init(bitsPerSample, channels, sampleRate, &handle);

                int frame = 0;
                do 
                {
                    _bytesWritten = file.decode(frame++, 0, 0, handle.buffer + bytesWritten, bufferSize - bytesWritten);
                    bytesWritten += _bytesWritten;
                } while(_bytesWritten >= 0);

                ao_play(handle.device, (char *) handle.buffer, bytesWritten);
            }
            break;
        case 3:
            {
                FFmpegFile file(input_filename);

                int64_t frames, samplesPerFrame, bitsPerSample, bufferSize, bytesWritten = 0, _bytesWritten;
                int channels, sampleRate;
                file.info(frames, samplesPerFrame, bitsPerSample, channels, sampleRate);

                bufferSize = frames * samplesPerFrame * bitsPerSample / 8;

                handle.buffer = (int8_t*) realloc(handle.buffer, bufferSize);

                init(bitsPerSample, channels, sampleRate, &handle);

                _bytesWritten = file.decodeSamples(1010000, 70000, handle.buffer + bytesWritten, bufferSize - bytesWritten);
                bytesWritten += _bytesWritten;

                _bytesWritten = file.decodeSamples(290000, 90000, handle.buffer + bytesWritten, bufferSize - bytesWritten);
                bytesWritten += _bytesWritten;

                _bytesWritten = file.decodeSamples(95000, 75000, handle.buffer + bytesWritten, bufferSize - bytesWritten);
                bytesWritten += _bytesWritten;

                ao_play(handle.device, (char *) handle.buffer, bytesWritten);
            }
            break;
    }
    
    free(handle.buffer);

    ao_shutdown();
  
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}