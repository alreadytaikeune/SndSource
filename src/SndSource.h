#ifndef _SNDSOURCE_H_
#define _SNDSOURCE_H_
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
#include "ringbuffer.h"
#include <mutex>


#define MAX_AUDIO_FRAME_SIZE 192000
#define SECONDS_IN_BUFFER 10
#define DEFAULT_OUT_SAMPLE_RATE  44100
#define DEFAULT_DATA_BUFFER_SIZE SECONDS_IN_BUFFER*DEFAULT_OUT_SAMPLE_RATE*2
#define PACKET_QUEUE_MAX_NB 1

typedef struct PacketQueue {
  AVPacketList *first_pkt, *last_pkt;
  int nb_packets;
  int nb_packets_max;
  int size;
  std::mutex queue_operation_mutex;
} PacketQueue;


typedef struct ConversionFormat{
	AVSampleFormat sample_fmt;
	int sample_rate;
	int64_t channel_layout;
} ConversionFormat;

typedef struct OpenOptions{
	std::string filename;
	int sample_rate=-1;
	long buffer_size=-1;
	int max_time=-1;
	int skip_time=-1;
	int nb_readers=1;
} OpenOptions;


static void options_copy(OpenOptions* p1, OpenOptions* p2){
	p2->filename =p1->filename;
	p2->sample_rate = p1->sample_rate;
	p2->buffer_size =  p1->buffer_size;
	p2->max_time=p1->max_time;
	p2->skip_time=p1->skip_time;
}

static void open_init(OpenOptions* p){
	p->filename = "";
	p->sample_rate = DEFAULT_OUT_SAMPLE_RATE;
	p->buffer_size = DEFAULT_DATA_BUFFER_SIZE;
	p->max_time=-1;
	p->skip_time=-1;
}

static void print_options(OpenOptions* op){
	std::cout << "filename: " << op->filename << std::endl;
	std::cout << "max: " << op->max_time << std::endl;
	std::cout << "skip: " << op->skip_time << std::endl;
	std::cout << "nb readers: " << op->nb_readers << std::endl;
	std::cout << "buffer size: " << op->buffer_size << std::endl;
}

class SndSource{
public:
	
	/*

		Main interface function, pulls size bytes from the ringbuffer. Returns the number of bytes read. 
		If < 0 the stream is finished, there is no more data to pull.
	*/
	int pull_data(uint8_t* buf, int size, int reader);

/*
             Frame
  |----------------------------|
  <--><--------------------><-->
  no             n            no (bytes)

  total_size of the frame: n+2*no
  
  Routine to iteratively read overlapping frames from the sound buffer through the SndSource interface.
  n: the size in bytes of the center of the frame
  no: the size in bytes of each of the trailing parts of the frame
  pulled_data: the array in which the frame will be stored. Must be at least n+2*no bytes long. 
  overlap: temporary array storing the back of the previous frame to be copied at the beginning of the current frame 
  being built. Should be at least no bytes long. It should be initialized with zeros for coherence. The routine takes in
  charge the subsequent fillings of the array, and it should be touched or freed before the reading is done.

  Returns the number of bytes that have actually been pulled from the audio buffer. If this number is 0, nothing is done 
  and 0 is returned. If this number is less than n+no, the frame is padded with zeros to always have the number of frames 
  usable for back-processing to be an integer.
*/
	int pull_data_overlap(int n, int no, int16_t* pulled_data, int16_t* overlap);

	SndSource(OpenOptions* op);
	~SndSource();

	int get_sampling();
	void reset(std::string f);
	std::string get_filename();
	int skip_seconds(float seconds);

	int get_time_in_bytes(float sec);
	int get_sample_rate();

protected:
	void open_stream();
	void packet_queue_init(PacketQueue *q, int nb);
	int fill_buffer();
	int fill_packet_queue();
	int audio_decode_frame(AVCodecContext *pCodecCtx, uint8_t *audio_buf, int buf_size);
	int dump_queue(size_t len);

private:

	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pCodecCtx = NULL; // the audio codec
	
	PacketQueue audioq;
	// IO formats of the conversion held in a structure, a bit redondant but more elegant
	ConversionFormat conversion_out_format;
	ConversionFormat conversion_in_format;
	int audioStreamId;
	OpenOptions* options;
	long max_byte;
	long bytes_read;
	bool no_more_packets=false;
	RingBuffer* data_buffer = (RingBuffer *) malloc(sizeof(RingBuffer));
	SwrContext *swr=NULL; // the resampling context for outputting with standard format in data_buffer
	// Condition for write availability in the data buffer not to do busy wait
	// in the dump_queue method
	pthread_mutex_t data_writable_mutex;

	// Condition for write availability in the data buffer not to do busy wait
	// in the dump_queue method
	pthread_cond_t data_writable_cond;


};

#endif