#ifndef _SNDWRITER_H_
#define _SNDWRITER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#ifdef __cplusplus 
}
#endif

#include <string>


typedef struct OutFormat{
	AVCodecID out_codec=AV_CODEC_ID_MP3;
	int bitrate = 64000;
	float sample_rate = 44100.f;
	AVSampleFormat sample_fmt= AV_SAMPLE_FMT_S16P;
	int channels = 1;
	int channel_layout=AV_CH_LAYOUT_MONO;

} OutFormat;


static void print_out_format(OutFormat& f){
	printf("codec: %d\nbitrate: %d\nsample_rate: %f\nsample_fmt: %d\nchannels: %d\nchannel_layout: %d\n",
		f.out_codec, f.bitrate, f.sample_rate, f.sample_fmt, f.channels, f.channel_layout);
}

class SndWriter{
public:
	SndWriter(std::string name, OutFormat* format);
	~SndWriter();
	int write(int16_t* frames, int frame_nb);
	int open();
	int close();

private:
	OutFormat* out_format;
	AVCodecContext *ctx;
	AVPacket pkt; 
	AVFrame* avframe;
	AVCodec *codec;
	FILE* f;
	std::string filename;
	uint8_t* out_buffer;
	int out_buffer_size;
	int idx=0;
	bool opened=false;
};

#endif