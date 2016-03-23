#include "SndSource.h"
#include <assert.h>
#include <stdlib.h>     /* exit, EXIT_FAILURE, size_t */

static int packet_queue_get(PacketQueue *q, AVPacket *pkt);
static int packet_queue_put(PacketQueue *q, AVPacket *pkt);


static bool is_full(PacketQueue* q){
	if(!q){
		std::cerr << "Packets queue is null" << std::endl;
		exit(EXIT_FAILURE);
	}
	if(q->nb_packets > q->nb_packets_max){
		std::cerr << "Inconherent state in queue, more packets than allowed" << std::endl;
		exit(EXIT_FAILURE);
	}
	return q->nb_packets == q->nb_packets_max;
	
}


static bool is_empty(PacketQueue* q){
	if(!q){
		std::cerr << "Packets queue is null" << std::endl;
		exit(EXIT_FAILURE);
	}
	return q->first_pkt == NULL;
}


static void queue_flush(PacketQueue *q){
	AVPacketList* pt = q->first_pkt;
	q->queue_operation_mutex.lock();
	while(pt){
		q->size-=q->first_pkt->pkt.size;
		q->first_pkt=pt->next;
		q->nb_packets--;
		av_free(pt);
		pt=q->first_pkt;
	}
	q->queue_operation_mutex.unlock();
	q->nb_packets=0;
	q->first_pkt=NULL;
	q->last_pkt=NULL;
}


static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
  if(is_full(q))
  	return -1;
  AVPacketList *pkt1;
  if(av_dup_packet(pkt) < 0) {
    return -1;
  }
  pkt1 = (AVPacketList*) av_malloc(sizeof(AVPacketList));
  if (!pkt1)
    return -1;
  pkt1->pkt = *pkt;
  pkt1->next = NULL;

  q->queue_operation_mutex.lock();
  
  if (!q->last_pkt)
    q->first_pkt = pkt1;
  else
    q->last_pkt->next = pkt1;
  q->last_pkt = pkt1;
  q->nb_packets++;
  q->size += pkt1->pkt.size;
  q->queue_operation_mutex.unlock();
  return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt) {
  AVPacketList *pkt1;
  int ret;
  q->queue_operation_mutex.lock();
  pkt1 = q->first_pkt;
  if (pkt1) {
    q->first_pkt = pkt1->next;
    if (!q->first_pkt)
	  q->last_pkt = NULL;
    q->nb_packets--;
    q->size -= pkt1->pkt.size;
    *pkt = pkt1->pkt;
    av_free(pkt1);
    ret = 1;
  } 
  else{
    ret = 0;
  }
  q->queue_operation_mutex.unlock();
  return ret;
}


void SndSource::packet_queue_init(PacketQueue *q, int nb_m) {
  memset(q, 0, sizeof(PacketQueue));
  q->nb_packets_max=nb_m;
}


SndSource::SndSource(OpenOptions* op){
	if(!op)
		options = (OpenOptions*) malloc(sizeof(OpenOptions));
	else
		options = op;
	print_options(options);
	max_byte=-1;
	bytes_read=0;
	if(options->sample_rate == -1){
		options->sample_rate = DEFAULT_OUT_SAMPLE_RATE;
	}

	if(options->buffer_size == -1){
		options->buffer_size = DEFAULT_DATA_BUFFER_SIZE;
	}

	packet_queue_init(&audioq, PACKET_QUEUE_MAX_NB);
	audioStreamId=-1;
	conversion_out_format.sample_fmt = AV_SAMPLE_FMT_S16;
	conversion_out_format.sample_rate = options->sample_rate;
	conversion_out_format.channel_layout = AV_CH_LAYOUT_MONO;
	swr = swr_alloc();
	int s = rb_create(data_buffer, options->buffer_size, options->nb_readers);
	if(s < 0){
		std::cerr << "Error creating ring buffer" << std::endl;
		exit(EXIT_FAILURE);
	}
	/* Initialize mutex and condition variable objects */
  	pthread_mutex_init(&data_writable_mutex, NULL);
  	pthread_cond_init (&data_writable_cond, NULL);

	if(options->max_time>0){
		max_byte = options->max_time*conversion_out_format.sample_rate*av_get_bytes_per_sample(conversion_out_format.sample_fmt);
	}

	open_stream();
	fill_buffer();

}

void SndSource::reset(std::string f){
	options->filename=f;
	if(options->max_time>0){
		max_byte = options->max_time*conversion_out_format.sample_rate*av_get_bytes_per_sample(conversion_out_format.sample_fmt);
	}	
	rb_reset(data_buffer);
	queue_flush(&audioq);
	avcodec_free_context(&pCodecCtx);
	swr_free(&swr);
	pFormatCtx=NULL;
	pCodecCtx=NULL;

	swr = swr_alloc();
}

SndSource::~SndSource(){
	queue_flush(&audioq);
	rb_free(data_buffer);
	swr_free(&swr);
	avformat_close_input(&pFormatCtx);
	pthread_mutex_destroy(&data_writable_mutex);
	pthread_cond_destroy(&data_writable_cond);
	avcodec_free_context(&pCodecCtx);
	
}

void SndSource::open_stream(){
	av_register_all();
	char* filename = (char*) options->filename.c_str();
	// Open video file
	if(avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0){
		std::cerr << "Error opening input" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0){
		std::cerr << "Error finding stream information" << std::endl;
		exit(EXIT_FAILURE);
	}

	av_dump_format(pFormatCtx, 0, filename, 0);

	int i;
	AVCodecContext *pCodecCtxOrig = NULL;

	// Find the first audio stream
	for(i=0; i<pFormatCtx->nb_streams; i++)
	  if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
     		audioStreamId < 0) {
	    audioStreamId=i;
	    break;
	  }
	if(audioStreamId==-1){
		std::cerr << "Didn't find audio stream" << std::endl;
		exit(EXIT_FAILURE);

	}

	// Get a pointer to the codec context for the audio stream
	pCodecCtxOrig=pFormatCtx->streams[audioStreamId]->codec;


	// Now find the actual codec and open it

	AVCodec *pCodec = NULL;

	// Find the decoder for the audio stream
	pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);

	if(pCodec==NULL) {
	  std::cerr << "Unsupported codec!" << std::endl;
	  exit(EXIT_FAILURE);
	}
	// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
	  std::cerr << "Couldn't copy codec context" << std::endl;
	  exit(EXIT_FAILURE);
	}
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec, NULL)<0){
	  std::cerr << " Could not open codec" << std::endl;
	  exit(EXIT_FAILURE);
	}

	if(pCodecCtx->sample_rate < options->sample_rate){
		printf("WARNING: the provided sample rate for opening (%dHz) is higher than the one of the input (%dHz). Using %dHz\n",
			options->sample_rate, pCodecCtx->sample_rate, pCodecCtx->sample_rate);
		options->sample_rate=pCodecCtx->sample_rate;
		conversion_out_format.sample_rate=pCodecCtx->sample_rate;
	}
	conversion_in_format.sample_fmt = pCodecCtx->sample_fmt;
	conversion_in_format.sample_rate = pCodecCtx->sample_rate;
	conversion_in_format.channel_layout = pCodecCtx->channel_layout;

	std::cout << "sample fmt " << conversion_in_format.sample_fmt << std::endl;
	std::cout << "sample rate " << conversion_in_format.sample_rate << std::endl;
	std::cout << "channel layout " << conversion_in_format.channel_layout << std::endl;
	if(conversion_in_format.channel_layout==0){
		if(pCodecCtx->channels == 1)
			conversion_in_format.channel_layout = AV_CH_LAYOUT_MONO;
		else
			conversion_in_format.channel_layout = AV_CH_LAYOUT_STEREO;
	}

	av_opt_set_int(swr, "in_channel_layout",  conversion_in_format.channel_layout, 0);
	av_opt_set_int(swr, "out_channel_layout", conversion_out_format.channel_layout,  0);
	av_opt_set_int(swr, "in_sample_rate",     conversion_in_format.sample_rate, 0);
	av_opt_set_int(swr, "out_sample_rate",    conversion_out_format.sample_rate, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt",  conversion_in_format.sample_fmt, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", conversion_out_format.sample_fmt,  0);
	swr_init(swr);
	if(options->skip_time != -1){
		int a = skip_seconds(options->skip_time);
	}

}


int SndSource::skip_seconds(float seconds){
	int bytes_to_skip=get_time_in_bytes(seconds);
	int skipped=0;
	fill_packet_queue();
	while(skipped<bytes_to_skip){
		skipped+=dump_queue(bytes_to_skip-skipped);
		if(rb_get_write_space(data_buffer)==0)
			rb_reset(data_buffer);
	}
	rb_reset(data_buffer);
	return skipped;
}

int SndSource::get_time_in_bytes(float sec){
	return (int) (sec*conversion_out_format.sample_rate*av_get_bytes_per_sample(conversion_out_format.sample_fmt));
}



/*
* Pulls from the queue, and waits if not enough data in the queue.
* Decodes and dumps the packet queue in a stream buffer
*/
int SndSource::dump_queue(size_t len) {
  //std::cout << "dumping queue" << std::endl;
  int len1, audio_size;
  static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
  static unsigned int audio_buf_size = 0;
  static unsigned int audio_buf_index = 0;
  int first_len = len; 
  int nb_written=0;
  // std::cout << "audio buf index " << audio_buf_index << std::endl;
  // std::cout << "audio buf size " << audio_buf_size << std::endl; 
  while(len > 0) {
    if(audio_buf_index >= audio_buf_size) {
      /* We have already sent all our data; get more */
      audio_size = audio_decode_frame(pCodecCtx, audio_buf, sizeof(audio_buf));
      if(audio_size < 0) { // EOF reached ?
	    /* If error, output silence */
	    if(no_more_packets)
	    	return 0;
	    audio_buf_size = 1024; // arbitrary?
	    memset(audio_buf, 0, audio_buf_size);
	    std::cout << "audio size < 0" << std::endl;
	    std::cout << audioq.nb_packets << std::endl;
      } 
      else {
	    audio_buf_size = audio_size;
      }
      audio_buf_index = 0;
    }
    // std::cout << "audio size read: " << audio_size << std::endl;
    len1 = audio_buf_size - audio_buf_index;
    if(len1 > len)
      len1 = len;
    nb_written=rb_write(data_buffer, (uint8_t *) audio_buf + audio_buf_index, len1);
    if(nb_written==0){
    	break;
    }
    len -= nb_written;
    audio_buf_index += nb_written;
  }

  return first_len - len;
}

/*
	Decode frames until being able to fill audio_buf or reach end of the available packets
	Returns the number of bytes written
*/
int SndSource::audio_decode_frame(AVCodecContext *pCodecCtx, uint8_t *audio_buf, int buf_size) {

  static AVPacket pkt;
  static uint8_t *audio_pkt_data = NULL;
  static int audio_pkt_size = 0;
  static AVFrame frame;
  static int count_in;
  static int bytes_per_sample = av_get_bytes_per_sample(conversion_out_format.sample_fmt); 
  int len1, data_size = 0;
  int nb_written = 0;
  for(;;) {
    while(audio_pkt_size > 0) {
      int got_frame = 0;
      len1 = avcodec_decode_audio4(pCodecCtx, &frame, &got_frame, &pkt);
      if(len1 < 0) {
	    /* if error, skip frame */
	    std::cerr << "error decoding frame" << std::endl;
	    audio_pkt_size = 0;
	    break;
      }
      audio_pkt_data += len1;
      audio_pkt_size -= len1;
      data_size = 0;
      nb_written = 0;
      if(got_frame){
	    data_size = av_samples_get_buffer_size(frame.linesize, 
					       pCodecCtx->channels,
					       frame.nb_samples,
					       pCodecCtx->sample_fmt,
					       1); // Total number of bytes pulled from the packet, not per channel!!
	    assert(data_size <= buf_size);

	    count_in = frame.nb_samples;
	    nb_written = swr_convert(swr, &audio_buf, buf_size, (const uint8_t**) frame.extended_data, count_in);
	    if(nb_written <= 0){
	    	std::cerr << "error converting data" << std::endl;
	    	exit(EXIT_FAILURE);
	    }
      }

      if(data_size <= 0) {
	    /* No data yet, get more frames */
	    continue;
      }
      /* We have data, return it and come back for more later */
      return nb_written*bytes_per_sample; 
    }
    if(pkt.data)
    	av_free_packet(&pkt);


    if(packet_queue_get(&audioq, &pkt) == 0){
    	if(fill_packet_queue()<=0){
    		std::cout << "fill packet queue <= 0 " << std::endl;
    		return -1;
    	}
    	packet_queue_get(&audioq, &pkt);
    }
    audio_pkt_data = pkt.data;
    audio_pkt_size = pkt.size;
  }
}



int SndSource::fill_buffer(){
	pthread_mutex_lock(&data_writable_mutex);

	int to_write = rb_get_write_space(data_buffer);
	int lread=0, len_written=0;
	fill_packet_queue();
	bool check_max=false;
	if(max_byte != -1){
		check_max=true;
		if(to_write > max_byte)
			to_write=max_byte;
	}
	
	while(to_write>len_written){
		if(check_max){
			if(max_byte<=0){
				break;
			}

			if(to_write > max_byte){
				to_write = max_byte;
			}
		}
		
		lread=dump_queue(to_write);
		if(check_max)
			max_byte-=lread;
		len_written+=lread;
		if(lread==0){
			if(fill_packet_queue() == -1){
				// there is no more packet to read, we reached the end of the data
				break;
			} 
		}
	}
	pthread_mutex_unlock(&data_writable_mutex);
	return len_written;
}


int SndSource::fill_packet_queue(){
	int nb=0;
	AVPacket packet;
	while(!is_full(&audioq)){
		int s = av_read_frame(pFormatCtx, &packet);
		while(s>=0) {
			if(packet.stream_index==audioStreamId) {
				if(packet_queue_put(&audioq, &packet)>=0){
					nb++;
				}
				else{
					std::cout << "impossible to put packet in queue" << std::endl;
				}
				break;
			}
			else {
	    		av_free_packet(&packet);
			}
			s=av_read_frame(pFormatCtx, &packet);
		}
		if(s<0){
			std::cout << "no more packets" << std::endl;
			no_more_packets=true;
			return -1;
		}
			
	}
	return nb;
}


int SndSource::pull_data(uint8_t* buf, int size, int reader){
	int len, lread, total_read;
	len=size;
	lread=0;
	total_read=0;
	while(len>0){
		lread=rb_read(data_buffer, (uint8_t *) buf+total_read, reader, (size_t) len); 
		len-=lread;
		total_read+=lread;
		if(lread==0){
			if(fill_buffer()<=0)
				break;
		}
	}
	return total_read;
}

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
int SndSource::pull_data_overlap(int n, int no, int16_t* pulled_data, int16_t* overlap){
  int total_read=0;
  total_read=pull_data((uint8_t*) pulled_data+no, n+no, 0); // writing pulled data after the part from previous frame
  if(total_read == 0){
    return 0;
  }

  if(total_read < n+no){
    memset((uint8_t*)pulled_data+total_read+no, 0, n+no-total_read); // padding with zeros
  }

  memcpy(pulled_data, overlap, no); // putting the back of the last frame at the beginning of this frame

  memcpy(overlap, (uint8_t*) pulled_data+no+n, no); // Putting the end of this frame in the temporary array that will be put at the front of the next frame

  return total_read;
}
