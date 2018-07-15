// SOUCE: https://blinkingblip.wordpress.com/2011/10/08/decoding-and-playing-an-audio-stream-using-libavcodec-libavformat-and-libao/

extern "C" {
    #include <stdio.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <ao/ao.h>
}
    
#define MAX_AUDIO_FRAME_SIZE 192000 
 
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
 
    // This call is necessarily done once in your app to initialize
    // libavformat to register all the muxers, demuxers and protocols.
    av_register_all();
 
    // A media container
    AVFormatContext* container = 0;
 
    if (avformat_open_input(&container, input_filename, NULL, NULL) < 0) {
        die("Could not open file");
    }
 
    if (avformat_find_stream_info(container, NULL) < 0) {
        die("Could not find file info");
    }
 
    int stream_id = -1;
 
    // To find the first audio stream. This process may not be necessary
    // if you can gurarantee that the container contains only the desired
    // audio stream
    int i;
    for (i = 0; i < container->nb_streams; i++) {
        if (container->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_id = i;
            break;
        }
    }
 
    if (stream_id == -1) {
        die("Could not find an audio stream");
    }
 
    // Extract some metadata
    AVDictionary* metadata = container->metadata;
 
    // Find the apropriate codec and open it
    AVCodecContext* codec_context = container->streams[stream_id]->codec;
 
    AVCodec* codec = avcodec_find_decoder(codec_context->codec_id);
 
    if (!avcodec_open2(codec_context, codec, NULL) < 0) {
        die("Could not find open the needed codec");
    }

    ao_initialize(); 
    int driver = ao_default_driver_id();

    AVPacket packet;
    int buffer_size = MAX_AUDIO_FRAME_SIZE;
    int8_t buffer[MAX_AUDIO_FRAME_SIZE];
    
    // Now seek back to the beginning of the stream
    //av_seek_frame(container, stream_id, 0, AVSEEK_FLAG_ANY);
 
    bool firstFrame = true;
    ao_device* device = NULL;
    while (1) {
        // Read one packet into `packet`
        if (av_read_frame(container, &packet) < 0) {
            break;  // End of stream. Done decoding.
        }

        if (firstFrame){
            ao_sample_format sample_format;

            if (codec_context->sample_fmt == AV_SAMPLE_FMT_U8) {
                sample_format.bits = 8;
            } else if (codec_context->sample_fmt == AV_SAMPLE_FMT_S16) {
                sample_format.bits = 16;
            } else if (codec_context->sample_fmt == AV_SAMPLE_FMT_S16P) {
                sample_format.bits = 16;
            } else if (codec_context->sample_fmt == AV_SAMPLE_FMT_S32) {
                sample_format.bits = 32;
            }
            
            sample_format.channels = codec_context->channels;
            sample_format.rate = codec_context->sample_rate;
            sample_format.byte_format = AO_FMT_NATIVE;
            sample_format.matrix = 0;

            // To initalize libao for playback
            device = ao_open_live(driver, &sample_format, NULL);
            firstFrame = false;
        }
 
        buffer_size = MAX_AUDIO_FRAME_SIZE;
 
        // Decodes from `packet` into the buffer
        if (avcodec_decode_audio3(codec_context, (int16_t*)buffer, &buffer_size, &packet) < 1) {
            break;  // Error in decoding
        }
 
        // Send the buffer contents to the audio device
        ao_play(device, (char*)buffer, buffer_size);
    }
 
    avformat_close_input(&container);
 
    ao_shutdown();
 
    fprintf(stdout, "Done playing. Exiting...");
 
    return 0;
}