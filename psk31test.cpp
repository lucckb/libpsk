//http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html
//gcc --std=gnu99 -o tutorial01 psk31test.c  -lavformat -lavcodec -lz -lavutil 

#include <stdint.h>
#include <functional>
#include <cstdio>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

}

#include <psk/decoder.hpp>

namespace _internal
{
	void on_new_samples( int16_t *sample, std::size_t count );
}

int main(int argc, const char * const *argv )
{
    if(argc < 2) 
    {

        printf("INvalid argument count\n");
        return -1;
    }
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, 0)!=0)
    {
        printf("Couldn't open file\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) <0)
    {
  
        printf("Couldn't find stream\n");
        return -1;
    }
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    int audioStream = -1;
    for( int i=0; i<pFormatCtx->nb_streams; i++)
    {
        if( pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO  )
        {
            audioStream = i;
            break;
        }
    }
    if(audioStream==-1)
    {
     printf("Couldn't find audio stream\n");
     return -1;
    }
    AVCodecContext *pCodecCtx = pFormatCtx->streams[audioStream]->codec;
    AVCodec *pCodec = avcodec_find_decoder( pCodecCtx->codec_id );
    if(pCodec == NULL)
    {
     printf("Couldn't find codec\n");
     return -1;
    }
     //bitstreams where frame boundaries can fall in the middle of packets
    if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
       pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    {
         printf("Couldn't open avcodec\n");
         return -1;
    }
    AVFrame *decoded_frame = avcodec_alloc_frame();
    AVPacket *avpkt = reinterpret_cast<AVPacket*>(malloc(sizeof(AVPacket)));
    ham::psk::decoder psk_dec( 8000 );
    while(av_read_frame(pFormatCtx, avpkt)>=0)
    {
        int got_frame = 0;
        int len = 0;
        {
            len = avcodec_decode_audio4(pCodecCtx, decoded_frame, &got_frame, avpkt );
            if(len < 0)
            {
                printf("Decode error\n");
                return -1;
            }
            if(got_frame)
            {
            	int data_size = av_samples_get_buffer_size(NULL,pCodecCtx->channels,
            	        decoded_frame->nb_samples, pCodecCtx->sample_fmt, 1);
            	psk_dec( reinterpret_cast<ham::psk::sample_type*>(decoded_frame->data[0]), data_size );
            }
        }
        if(len < 0)
        {
            printf("Error when decoding\n");
            return -1;
        }
    }
    free( avpkt );
    free( decoded_frame );
    return 0;
}


