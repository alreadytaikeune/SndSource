#include "SndWriter.h"
#include <iostream>

SndWriter::SndWriter(std::string name, OutFormat* format){
	filename=name;
	out_format=format;
	idx=0;
}

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat *p = codec->sample_fmts;
	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == sample_fmt)
			return 1;
		p++;
	}
	return 0;
}

int SndWriter::write(uint8_t* buf, int n){
	if(!opened){
		std::cerr << "[ERROR]: stream not opened" << std::endl;
		exit(1);
	}

	int ret=0;
	int got_output;
	for(int i=0;i<n;i++){
		out_buffer[idx] = buf[i];
		idx++;
		if(idx==ctx->frame_size*2){
			av_init_packet(&pkt);
    		pkt.data = NULL; // packet data will be allocated by the encoder
			pkt.size = 0;
			ret = avcodec_encode_audio2(ctx, &pkt, avframe, &got_output);
		    if (ret < 0) {
				fprintf(stderr, "Error encoding audio frame\n");
		    	exit(1);
		    }
		    if (got_output) {
		        fwrite(pkt.data, 1, pkt.size, f);
		        av_free_packet(&pkt);
		    }
		    idx=0;
		}
	}
}

int SndWriter::open(){
	codec = avcodec_find_encoder(out_format->out_codec);
	ctx = avcodec_alloc_context3(codec);
	ctx->bit_rate=out_format->bitrate;
	ctx->channels = out_format->channels;
	ctx->sample_rate = out_format->sample_rate;
	ctx->sample_fmt= out_format->sample_fmt;
	ctx->channel_layout=out_format->channel_layout;
	avframe=av_frame_alloc();
	f = fopen(filename.c_str(), "wb");

	if (!check_sample_fmt(codec, ctx->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s\n",
        av_get_sample_fmt_name(ctx->sample_fmt));
		exit(1);
    }

	if (avcodec_open2(ctx, codec, NULL) < 0) {
    	fprintf(stderr, "could not open codec\n");
    	exit(1);
    }


    FILE *f = fopen(filename.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "could not open out file\n");
        exit(1);
    }

	avframe->nb_samples     = ctx->frame_size;
	avframe->format     	= ctx->sample_fmt;
	avframe->channel_layout = ctx->channel_layout;
	print_out_format(*out_format);
	

	out_buffer_size=av_samples_get_buffer_size(NULL, ctx->channels, ctx->frame_size, ctx->sample_fmt, 0);
	out_buffer = (uint8_t*) malloc(out_buffer_size);
	std::cout << out_buffer_size << std::endl;
	std::cout << ctx->channels << std::endl;
	std::cout << ctx->frame_size << std::endl;

	std::cout << ctx->sample_fmt << std::endl;

	int ret = avcodec_fill_audio_frame(avframe, ctx->channels, ctx->sample_fmt,
        (const uint8_t*)out_buffer, out_buffer_size, 0);

	if (ret < 0) {
		fprintf(stderr, "Could not setup audio frame, error %d\n", ret);
		exit(1);
	}
	opened=true;
}

int SndWriter::close(){
	int ret;
	int i=0,got_output=1;
	for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_audio2(ctx, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
 
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
    fclose(f);

}


SndWriter::~SndWriter(){
	free(out_buffer);
	av_frame_free(&avframe);
	avcodec_free_context(&ctx);
}