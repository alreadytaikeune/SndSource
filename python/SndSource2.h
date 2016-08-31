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

#define THREAD_SAFE 0


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
  #if THREAD_SAFE
  std::mutex queue_operation_mutex;
  #endif
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
    
    int pull_data(short* IN_ARRAY1, int DIM1, int reader);


    int pull_data_overlap(int n, int no, int16_t* pulled_data, int16_t* overlap);

    SndSource(OpenOptions* op);
    ~SndSource();

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
    #if THREAD_SAFE
    // Condition for write availability in the data buffer not to do busy wait
    // in the dump_queue method
    pthread_mutex_t data_writable_mutex;

    // Condition for write availability in the data buffer not to do busy wait
    // in the dump_queue method
    pthread_cond_t data_writable_cond;
    #endif


};

#endif