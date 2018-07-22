// REFERENCES:
// http://ffmpeg.org/doxygen/trunk/doc_2examples_2decoding__encoding_8c-example.html
// https://blinkingblip.wordpress.com/2011/10/08/decoding-and-playing-an-audio-stream-using-libavcodec-libavformat-and-libao/

extern "C" {
    #include <stdio.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <ao/ao.h>
}
    
#define MAX_AUDIO_FRAME_SIZE 192000 

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

typedef void (*init_playback_callback)(int bits, int channels, int sample_rate, void * data);
typedef void (*play_callback)(char * buffer, int bufferSize, void * data);

/*
 * Audio decoding.
 */
static void audio_decode_example(const char *filename, init_playback_callback init_playback, play_callback play, void * data)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int len;
    FILE *f;
    uint8_t inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;

    av_register_all();
    ao_initialize(); 

    int driver = ao_default_driver_id();
    bool firstFrame = true;
    ao_device* device = NULL;

    av_init_packet(&avpkt);

    /* find the mpeg audio decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    /* decode until eof */
    avpkt.data = inbuf;
    avpkt.size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);

    while (avpkt.size > 0) {
        int got_frame = 0;

        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        } else
            av_frame_unref(decoded_frame);

        len = avcodec_decode_audio4(c, decoded_frame, &got_frame, &avpkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding\n");
            exit(1);
        }
        if (got_frame) {
            if (firstFrame){
                int bits = -1;

                if (c->sample_fmt == AV_SAMPLE_FMT_U8) {
                    bits = 8;
                } else if (c->sample_fmt == AV_SAMPLE_FMT_S16) {
                    bits = 16;
                } else if (c->sample_fmt == AV_SAMPLE_FMT_S16P) {
                    bits = 16;
                } else if (c->sample_fmt == AV_SAMPLE_FMT_S32) {
                    bits = 32;
                }

                init_playback(bits, c->channels, c->sample_rate, data);

                firstFrame = false;
            }

            /* if a frame has been decoded, output it */
            int data_size = av_samples_get_buffer_size(NULL, c->channels,
                                                       decoded_frame->nb_samples,
                                                       c->sample_fmt, 1);

            play((char*)decoded_frame->data[0], data_size, data);
        }
        avpkt.size -= len;
        avpkt.data += len;
        avpkt.dts =
        avpkt.pts = AV_NOPTS_VALUE;
        if (avpkt.size < AUDIO_REFILL_THRESH) {
            /* Refill the input buffer, to avoid trying to decode
             * incomplete frames. Instead of this, one could also use
             * a parser, or use a proper container format through
             * libavformat. */
            memmove(inbuf, avpkt.data, avpkt.size);
            avpkt.data = inbuf;
            len = fread(avpkt.data + avpkt.size, 1,
                        AUDIO_INBUF_SIZE - avpkt.size, f);
            if (len > 0)
                avpkt.size += len;
        }
    }

    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&decoded_frame);

    ao_shutdown();
}
