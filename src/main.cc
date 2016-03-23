#include "SndSource.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;


void print_array(int16_t* data, int n){
  for(int i=0; i<n; i++)
    std::cout << data[i] << " ";
  std::cout << std::endl;
}


void print_arrayf(float* data, int n){
  for(int i=0; i<n; i++)
    std::cout << data[i] << " ";
  std::cout << std::endl;
}

void read_data(SndSource& src){

	int window_size=2048;
	int16_t* pulled_data = (int16_t *) malloc(window_size*sizeof(int16_t));
	int total_read=0;

	while(1){
		total_read=src.pull_data((uint8_t*) pulled_data, (int) window_size*2, 0);
		if(total_read == 0){
			break;
		}

    // DO THINGS
	}

	free(pulled_data);
}


float compute_error(float* v1, float* v2, int n){
  float err=0.f;
  for(int i=0;i<n;i++)
    err+=(v1[i]-v2[i])*(v1[i]-v2[i]);
  return err;
}


// i = 1 for ramp up, -1 for ramp down

void apply_ramp(float* buf, int i, int n){
  int l=n-1;
  int j=0;
  for(float k=0.5*(l-i*l);k<n && k>=0;k+=i){
    buf[j]*=(k/(n-1));
    j++;
  }
}


/*
  Can be easily optimized with SIMD instructions
*/
void raw_to_flt(int16_t* in, float* out, int n){
  for(int i=0;i<n;i++){
    out[i] = in[i]/32758.f;
  }
}


/*
  Routine to be called when wanted to iteratively merge overlapping frames that have been extracted with read_overlap for
  instance. 
  
  n: inner frame size in bytes
  no: padding size in bytes
  out: float vector of minimum size n+no
  flt_data: the vector from which to copy the frames. It is not modified by this routine. The first n+no bytes of this 
  vector are copied in the output vector in the following way:
    1 - the first no bytes are merged with the last no bytes of the previous frame stored in the overlap vector, by 
    applying a ramp up to the first ones and a ramp down to the last ones and then summing them. 
    2 - the next n bytes are simply copied in the output buffer, resulting in an n+no bytes frame
  The last no bytes of the flt_data vector are finally simply copied to the overlap vector for subsequent calls.

  frame_size: the size in bytes of a data frame (2 for 16 bits raw PCM data)


*/

void merge_frames(float* out, float* flt_data, int n, int no, int frame_size, float* overlap){
  n/=frame_size;
  no/=frame_size;
  memcpy(out, flt_data, no*sizeof(float));
  apply_ramp(out, 1, no);
  apply_ramp(overlap, -1, no);
  for(int i=0;i<no;i++)
    out[i]+=overlap[i];
  memcpy(out+no, flt_data+no, n*sizeof(float));
  memcpy(overlap, flt_data+n+no, no*sizeof(float));

}



void read_data_overlap(SndSource& src, float time_frame, float time_overlap){
  int n = src.get_time_in_bytes(time_frame);
  int no = src.get_time_in_bytes(time_overlap);
  int frame_size = sizeof(int16_t);
  int frame_nb=(n+2*no)/frame_size; 
  int16_t* pulled_data = (int16_t *) malloc(n+2*no);
  float* flt_data = (float *) malloc(frame_nb*sizeof(float));
  int16_t* overlap = (int16_t *) malloc(no);
  float* merge_overlap = (float*) calloc(no/frame_size, sizeof(float));
  float* out = (float*) calloc((n+no)/frame_size, sizeof(float));
  memset(pulled_data, 0, no);
  memset(overlap, 0, no);
  int total_read=0;
  while(1){
    total_read=src.pull_data_overlap(n, no, pulled_data, overlap);
    if(total_read == 0){
      break;
    }
    raw_to_flt(pulled_data, flt_data, frame_nb);
    merge_frames(out, flt_data, n, no, frame_size, merge_overlap);
  }

  free(flt_data);
  free(pulled_data);
  free(overlap);
  free(out);
  free(merge_overlap);
}



void lpc_filter(SndSource& src, float time_frame, float time_overlap, int order){
  int n = src.get_time_in_bytes(time_frame);
  int no = src.get_time_in_bytes(time_overlap);
  int frame_size = sizeof(int16_t);
  int frame_nb=(n+2*no)/frame_size; 
  int16_t* pulled_data = (int16_t *) malloc(n+2*no);
  float* flt_data = (float *) malloc(frame_nb*sizeof(float));
  int16_t* overlap = (int16_t *) malloc(no);
  float* merge_overlap = (float*) calloc(no/frame_size, sizeof(float));
  float* out = (float*) calloc((n+no)/frame_size, sizeof(float));
  memset(pulled_data, 0, no);
  memset(overlap, 0, no);
  int total_read=0;
  while(1){
    total_read=src.pull_data_overlap(n, no, pulled_data, overlap);
    if(total_read == 0){
      break;
    }
    raw_to_flt(pulled_data, flt_data, frame_nb);
    merge_frames(out, flt_data, n, no, frame_size, merge_overlap);
  }

  free(flt_data);
  free(pulled_data);
  free(overlap);
  free(out);
  free(merge_overlap);
}


int main(int argc, char *argv[]){
	OpenOptions op;
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produces help message")
    	("input,i", po::value(&op.filename), "the names of the input files")
  		("max", po::value(&op.max_time), "the maximum number of seconds to take into account")
    	("skip", po::value(&op.skip_time), "number of seconds to skip at the beginning")
      ("rate", po::value(&op.sample_rate), "number of seconds to skip at the beginning")

    ;

  po::positional_options_description p;
  p.add("input", -1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
  po::notify(vm);
	SndSource src(&op);
  read_data_overlap(src, 0.02, 0.005);

	return 0;
}