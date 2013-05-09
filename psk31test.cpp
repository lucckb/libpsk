//http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html
//gcc --std=gnu99 -o tutorial01 psk31test.c  -lavformat -lavcodec -lz -lavutil 
//ffplay -f f64le -ar 8000 -channels 1 ~/worksrc/workspace/psk31/test.raw
#include <limits>
#include <cmath>
#include <psk/decoder.hpp>
#include <psk/modulator.hpp>
#include <functional>



#ifndef __arm__
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
#include <unistd.h>

namespace 
{
	class sample_tester
    {
   public:
		void decoder_callback( int event, int param, int param2  )
		{
			using namespace ham::psk;
			using namespace std;
			switch( event )
			{
			case decoder::cb_rxchar:
				cout << "GOT char " << char(param) << endl;
				break;
			case decoder::cb_clkerror:
				cout << "CLKERR value " << param << endl;
				break;
			case decoder::cb_imdrdy:
				cout << "IMDRDY value " << param << " " << param2 << endl;
				break;
			default:
				cout << "Unknown evt" << endl;
			}
		}
    public:
        sample_tester()  :
        	 psk_dec( 8000, std::bind( &sample_tester::decoder_callback,
        			this, std::placeholders::_1, std::placeholders::_2 , std::placeholders::_3) )
            , sample_min( std::numeric_limits<double>::max() )
            , sample_max( std::numeric_limits<double>::min() )
    		, wfile(nullptr)
        {
            psk_dec.set_mode( ham::psk::decoder::mode::qpsku, ham::psk::decoder::baudrate::b31 );
            psk_dec.set_frequency( 2125 );
            //psk_dec.set_squelch_tresh( 50, ham::psk::squelch_mode::fast );
            psk_dec.set_afc_limit( 100 ); //TMP
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
        int on_new_samples( int16_t *sample, size_t buflen )
        {
            for( size_t i=0; i<buflen; i++)
            {
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




int decoder_main( const char *filename )
{

    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // Open video file
    if(avformat_open_input(&pFormatCtx, filename, NULL, 0)!=0)
    {
        printf("Couldn't open file\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) <0)
    {
  
        printf("Couldn't find stream\n");
        return -1;
    }
    av_dump_format(pFormatCtx, 0, filename , 0);
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
      AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;
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
            	stester.on_new_samples( reinterpret_cast<int16_t*>(dst_data[0]), dst_bufsize / sizeof(int16_t) );
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

/* *************************** ENCODER ********************************** */


class audio_writer
{
private:

	/* Add an output stream. */
	AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
								enum AVCodecID codec_id)
	{
		AVStream *st;
		AVCodecContext *c;
		/* find the encoder */
		*codec = avcodec_find_encoder(codec_id);
			if (!(*codec)) {
				fprintf(stderr, "Could not find encoder for '%s'\n",
					avcodec_get_name(codec_id));
				exit(1);
		}
		st = avformat_new_stream(oc, *codec);
		if (!st) {
			fprintf(stderr, "Could not allocate stream\n");
			exit(1);
		}
		st->id = oc->nb_streams-1;
		c = st->codec;
		switch ((*codec)->type) {
			case AVMEDIA_TYPE_AUDIO:
				st->id = 1;
				c->sample_fmt = AV_SAMPLE_FMT_S16;
				c->bit_rate = 64000;
				sample_rate = c->sample_rate = 8000;
				n_channels = c->channels = 1;
				break;
			case AVMEDIA_TYPE_VIDEO:
				printf("AVMEDIA_TYPE_VIDEO VIdeo not supported\n"); exit(-1);
				break;
			default:
				break;
		}
		/* Some formats want stream headers to be separate. */
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		return st;
	}

	void write_audio_frame(AVFormatContext *oc, AVStream *st)
	{
		AVCodecContext *c;
		AVPacket pkt;
        memset(&pkt, 0, sizeof pkt );
		AVFrame *frame = avcodec_alloc_frame();
		int got_packet, ret;
		av_init_packet(&pkt);
		c = st->codec;
		frame->nb_samples = audio_input_frame_size;
		avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
								 (uint8_t *)samples,
								 audio_input_frame_size *
								 av_get_bytes_per_sample(c->sample_fmt) *
								 c->channels, 1);
		ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
		if (ret < 0) {
			fprintf(stderr, "Error encoding audio frame: %i\n",ret);
			exit(1);
		}
		if (!got_packet)
			return;
		pkt.stream_index = st->index;
		/* Write the compressed frame to the media file. */
		ret = av_interleaved_write_frame(oc, &pkt);
		if (ret != 0) {
			fprintf(stderr, "Error while writing audio frame: %i\n",
					ret);
			exit(1);
		}
		avcodec_free_frame(&frame);
	}
	void close_audio(AVFormatContext * /*oc*/, AVStream *st)
	{
		avcodec_close(st->codec);
		av_free(samples);
	}
	void open_audio(AVFormatContext * /*oc*/, AVCodec *codec, AVStream *st)
	{
		AVCodecContext *c;
		int ret;
		c = st->codec;
		/* open it */
		ret = avcodec_open2(c, codec, NULL);
		if (ret < 0) {
			fprintf(stderr, "Could not open audio codec: %i\n", ret );
			exit(1);
		}

		if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
			audio_input_frame_size = 1024;
		else
			audio_input_frame_size = c->frame_size;
		samples = (int16_t*) av_malloc(audio_input_frame_size *
		av_get_bytes_per_sample(c->sample_fmt) * c->channels);
		if (!samples) {
			fprintf(stderr, "Could not allocate audio samples buffer\n");
			exit(1);
		}
	}
public:
	audio_writer( const char *filename )
	{
		/* Initialize libavcodec, and register all codecs and formats. */
		av_register_all();
		/* allocate the output media context */
		avformat_alloc_output_context2(&oc, NULL, NULL, filename);
		if (!oc) {
			printf("Could not deduce output format from file extension: using MPEG.\n");
			avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
		}
		if (!oc) {
			throw std::logic_error("Unable to allocate oc");
		}
		fmt = oc->oformat;
		if (fmt->audio_codec != AV_CODEC_ID_NONE)
		{
			audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);
		}
		if (audio_st)
			open_audio(oc, audio_codec, audio_st);

		av_dump_format(oc, 0, filename, 1);
		/* open the output file, if needed */
		if (!(fmt->flags & AVFMT_NOFILE)) {
			int ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
			if (ret < 0) {
				fprintf(stderr, "Could not open '%s': %i\n", filename,
						ret);
				throw std::logic_error("Unable to open the file");
			}
		}
		/* Write the stream header, if any. */
		int ret = avformat_write_header(oc, NULL);
		if (ret < 0)
		{
			fprintf(stderr, "Error occurred when opening output file: %i\n",
					ret);
			throw std::logic_error("Unable to open the file 2");
		}
		if (frame)
			frame->pts = 0;
	}
	//Write audio data
	void write_audio_data( )
	{
		write_audio_frame( oc, audio_st );
	}
	int16_t* get_sample_buf() const
	{
		return samples;
	}
	size_t get_audio_frame_size() const
	{
		return audio_input_frame_size;
	}
	int get_channels() const
	{
		return n_channels;
	}

	~audio_writer()
	{
		/* Write the trailer, if any. The trailer must be written before you
		 * close the CodecContexts open when you wrote the header; otherwise
		 * av_write_trailer() may try to use memory that was freed on
		 * av_codec_close(). */
		av_write_trailer(oc);
		if (audio_st)
			close_audio(oc, audio_st);
		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_close(oc->pb);
		/* free the stream */
		avformat_free_context(oc);
	}
	double get_pts() const
	{
		double audio_pts;
		if (audio_st)
			audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
		else
			audio_pts = 0.0;
		return audio_pts;
	}
	int get_samplerate() const
	{
		return sample_rate;
	}
private:
	AVFormatContext *oc {};
	AVOutputFormat *fmt {};
	AVCodec *audio_codec {} ;
	AVStream *audio_st {} ;
	int16_t *samples {} ;
	int audio_input_frame_size {};
	AVFrame *frame {};
	int sample_rate {} ;
	int n_channels {};
};




int encoder_main( const char *filename )
{
	class audio_writer ww( filename );
	ham::psk::modulator mod( ww.get_samplerate(),  2125, 1024 );
	const char txt[] = "Ala ma kota";
	mod.set_mode( ham::psk::modulator::mode::qpsku, ham::psk::modulator::baudrate::b31);
	for(size_t i=0;i<sizeof txt - 1; i++)
		mod.put_tx( txt[i] );
	for (;;) {
		/* Compute current audio and video time. */
		mod( ww.get_sample_buf(), ww.get_audio_frame_size() );
		//gen_audio_frame( ww.get_sample_buf(), ww.get_audio_frame_size(), ww.get_channels() );
		ww.write_audio_data();
		if( mod.get_state() == ham::psk::modulator::state::off )
			break;
	}
	return 0;
}


/* *************************** MAIN LOOP ********************************** */

int main(int argc, const char * const *argv )
{
	 if(argc < 2)
	 {
	    printf("INvalid argument count\n");
	    return -1;
	 }
	 if( argc == 2 )
	 {
		 printf("Decode file %s\n", argv[1] );
		 return decoder_main( argv[1] );
	 }
	 else if( argc == 3 || !strcmp( argv[1],"--encode") )
	 {
		 if( access( argv[2], R_OK ) == 0 )
		 {
			 printf("File %s already exists. Are you sure to overvrite ? [n/Y] ", argv[2]);
			 char ans = 'n';
			 scanf( "%c", &ans );
			 if( !(ans == 'Y' || ans == 'y') )
			 {
				 printf("Aborting\n");
				 return -1;
			 }
		 }
		 printf("Encode file %s\n", argv[2] );
		 return encoder_main( argv[2] );
	 }
	 else
	 {
		 printf("Parameters parsing error !!!!!");
		 return -1;
	 }
}



#else /*ARM defined test only */

void decoder_callback( int event, int param, int param2  )
{

}

int main( )
{
	ham::psk::decoder psk_dec( 8000, decoder_callback );
	static int16_t sample_buf[1024];
	psk_dec( sample_buf, 1024 );
}

#endif
