//http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html
//gcc --std=gnu99 -o tutorial01 psk31test.c  -lavformat -lavcodec -lz -lavutil 
//ffplay -f f64le -ar 8000 -channels 1 ~/worksrc/workspace/psk31/test.raw
#include <stdint.h>
#include <functional>
#include <cstdio>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#include <limits>
#include <psk/decoder.hpp>


namespace 
{
    class sample_tester
    {
    public:
        sample_tester()  : psk_dec( 8000 )
            , sample_min( std::numeric_limits<double>::max() )
            , sample_max( std::numeric_limits<double>::min() )
    		, wfile(nullptr)
        {
            psk_dec.set_mode( ham::psk::decoder::mode::qpsku, ham::psk::decoder::baudrate::b63 );
            psk_dec.set_frequency( 999 );
            //psk_dec.set_squelch_tresh( 50, ham::psk::decoder::squelch_mode::fast );
            psk_dec.set_afc_limit( 100 );
		#if 0
			wfile = fopen("test.raw","wb");
			if( !wfile )
			{
				printf("Cant open file!!");
			}
		#endif
         }
        ~sample_tester()
        {
        	if(wfile) fclose(wfile);
        }
        int on_new_samples( double *sample, size_t buflen )
        {
            for( size_t i=0; i<buflen; i++)
            {
                sample[i] *= double(1<<15);
            	if( sample[i] > sample_max) sample_max = sample[i];
                if( sample[i] < sample_min ) sample_min = sample[i];
            }
            psk_dec( sample, buflen );
            if(wfile)
                fwrite( sample, sizeof(double), buflen, wfile );
            return 0;
        }
        void on_complete()
        {
           printf("CURR F %i LEV %i\n", psk_dec.get_frequency(), psk_dec.get_signal_level() );
           std::cout << "Sample min " << sample_min << " Sample max " << sample_max << std::endl;
        }
    private:
       ham::psk::decoder psk_dec;
       double sample_min;
       double sample_max;
       FILE *wfile;
    };
}


namespace
{
	int alloc_samples_array_and_data(uint8_t ***data, int *linesize, int nb_channels,
	int nb_samples, enum AVSampleFormat sample_fmt, int align)
	{
		int nb_planes = av_sample_fmt_is_planar(sample_fmt) ? nb_channels : 1;
		*data = (uint8_t**)av_malloc(sizeof(*data) * nb_planes);
		if (!*data)
			return AVERROR(ENOMEM);
		return av_samples_alloc(*data, linesize, nb_channels, nb_samples, sample_fmt, align);
}


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
    for( unsigned i=0; i<pFormatCtx->nb_streams; i++)
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

    //Init resample context
    struct SwrContext *swr_ctx = swr_alloc();
    if (!swr_ctx)
    {
		fprintf(stderr, "Could not allocate resampler context\n");
		return AVERROR(ENOMEM);
    }
    /* set options */
      const int dst_rate = 8000;
      const int dst_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
      AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_DBL;
      av_opt_set_int(swr_ctx, "in_sample_rate", pCodecCtx->sample_rate, 0);
      av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", pCodecCtx->sample_fmt, 0);
      av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
      av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
      av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);


    uint8_t **dst_data = NULL;
    AVFrame *decoded_frame = avcodec_alloc_frame();
    AVPacket *avpkt = reinterpret_cast<AVPacket*>(malloc(sizeof(AVPacket)));
    av_init_packet( avpkt );
    sample_tester stester;
    
    printf("Sample rate %i\n", pCodecCtx->sample_rate );
    int m_resample_initalized = false;
    int  dst_linesize;

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
                if( !m_resample_initalized )
                {
                	av_opt_set_int(swr_ctx, "in_channel_layout", av_frame_get_channel_layout(decoded_frame), 0);
                	av_opt_set_int(swr_ctx, "in_channel_count", av_frame_get_channels(decoded_frame), 0);
                	m_resample_initalized = true;
                	 /* initialize the resampling context */
                	  if ((swr_init(swr_ctx)) < 0)
                	  {
                		  fprintf(stderr, "Failed to initialize the resampling context\n");
                	  	 return -1;
                	  }
                	  else
                	  {
                		  printf("SWR init OK\n");
                	  }
                }
            	av_samples_get_buffer_size(NULL,pCodecCtx->channels,
            	        decoded_frame->nb_samples, pCodecCtx->sample_fmt, 1);
            	//printf("Try decode %i %i\n" , data_size, 	pCodecCtx->sample_fmt );
            	 /* compute the number of converted samples: buffering is avoided
            	* ensuring that the output buffer will contain at least all the
            	* converted input samples */
            	int dst_nb_samples =
            			av_rescale_rnd(decoded_frame->nb_samples, dst_rate, pCodecCtx->sample_rate, AV_ROUND_UP);

            	if( !dst_data )
            	{
            		int ret = alloc_samples_array_and_data(&dst_data, &dst_linesize, dst_nb_channels,
            				dst_nb_samples, dst_sample_fmt, 0);
            		if (ret < 0) {
            			fprintf(stderr, "Could not allocate destination samples\n");
            			return -1;
            		}

            	}
            	 /* convert to destination format */
            	int ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,
            			(const uint8_t**)decoded_frame->data, decoded_frame->nb_samples );
            	if (ret < 0) {
            		fprintf(stderr, "Error while converting\n");
            		return -1;
            	}
            	 int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
            			 ret, dst_sample_fmt, 1);
            	stester.on_new_samples( reinterpret_cast<double*>(dst_data[0]), dst_bufsize / sizeof(double) );
            	//printf("Processs samples %lu\n", dst_bufsize / sizeof(double));
            }
        }
        if(len < 0)
        {
            printf("Error when decoding\n");
            return -1;
        }
    }
    stester.on_complete();
    free( avpkt );
    free( decoded_frame );
    return 0;
}


