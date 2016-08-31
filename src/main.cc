#include "SndSource.h"
#include <boost/program_options.hpp>
#include "lpc.h"
#include "SndWriter.h"
#include <assert.h>
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

void read_data(SndSource& src, SndWriter& writer){

	int window_size=2048;
	int16_t* pulled_data = (int16_t *) malloc(window_size*sizeof(int16_t));
	int total_read=0;

	while(1){
		total_read=src.pull_data(pulled_data, (int) window_size, 0);
		if(total_read == 0){
			break;
		}

    writer.write((uint8_t *) pulled_data, window_size);
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
  Can be easily optimized with SIMD instructions
*/
void flt_to_raw(float* in, int16_t* out, int n){
  for(int i=0;i<n;i++){
    out[i] = int16_t(in[i]*32758.f);
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
  apply_ramp(overlap, -1, no);
  apply_ramp(out, 1, no);
  for(int i=0;i<no;i++)
    out[i]+=overlap[i];
  memcpy(out+no, flt_data+no, n*sizeof(float));
  memcpy(overlap, flt_data+n+no, no*sizeof(float));

}


void diff_vec(float* in, float* out, int n){
  for(int k=0; k<n-1; k++)
    out[k] = in[k+1]-in[k];
}


void scale(float* in, float e, int n){
  for(int k=0;k<n;k++)
    in[k]*=e;
}

void read_data_overlap(SndSource& src, SndWriter& writer, float time_frame, float time_overlap){
  int n = src.get_time_in_bytes(time_frame);
  int no = src.get_time_in_bytes(time_overlap);
  if(n%2==1)
    n++;
  if(no%2==1)
    no++;
  std::cout << "n: " << n << std::endl;
  std::cout << "no: " << no << std::endl;
  int frame_size = sizeof(int16_t);
  int frame_nb=(n+2*no)/frame_size; 
  int16_t* pulled_data = (int16_t *) malloc(n+2*no);
  float* flt_data = (float *) malloc(frame_nb*sizeof(float));
  int16_t* overlap = (int16_t *) malloc(no);
  float* merge_overlap = (float*) calloc(no/frame_size, sizeof(float));
  float* out = (float*) calloc((n+no)/frame_size, sizeof(float));
  int16_t* to_write = (int16_t *) malloc(n+no);
  memset(pulled_data, 0, no);
  memset(overlap, 0, no);
  int total_read=0;
  while(1){
    total_read=src.pull_data_overlap(n, no, pulled_data, overlap);
    if(total_read == 0){
      break;
    }
    raw_to_flt(pulled_data, flt_data, frame_nb);

    std::cout << "flt data" << std::endl;
    for(int i=0;i<20;i++)
      std::cout << flt_data[i+no/2] << " ";
    std::cout << std::endl;
    merge_frames(out, flt_data, n, no, frame_size, merge_overlap);
    std::cout << "out" << std::endl;
    for(int i=0;i<20;i++)
      std::cout << out[i+no/2] << " ";
    std::cout << std::endl;
    flt_to_raw(out, to_write, (n+no)/frame_size);

    writer.write((uint8_t *) to_write, (n+no));
  }

  free(flt_data);
  free(pulled_data);
  free(overlap);
  free(out);
  free(merge_overlap);
  free(to_write);
}



void lpc_filter(SndSource& lpcsrc, SndSource& exsrc, SndWriter& writer, float time_frame, 
  float time_overlap, int lpc_order){
  int n = lpcsrc.get_time_in_bytes(time_frame);
  int no = exsrc.get_time_in_bytes(time_overlap);
   if(n%2==1)
    n++;
  if(no%2==1)
    no++;
  assert(LPC_MAX_ORDER > lpc_order);
  float coefs[LPC_MAX_ORDER+1];
  memset(coefs, 0, (lpc_order+1)*sizeof(float));
  int frame_size = sizeof(int16_t);
  int frame_nb=(n+2*no)/frame_size;

  int16_t* pulled_data = (int16_t *) malloc(n+2*no);
  int16_t* pulled_data2 = (int16_t *) malloc(n+2*no);
  int16_t* to_write = (int16_t *) malloc(n+no);

  float* flt_data = (float *) malloc(frame_nb*sizeof(float));
  float* diff = (float *) malloc(frame_nb*sizeof(float));
  float* synthetized = (float *) malloc(frame_nb*sizeof(float));

  int16_t* overlap = (int16_t *) malloc(no+1);
  int16_t* overlap2 = (int16_t *) malloc(no+1);

  float* merge_overlap = (float*) calloc(no/frame_size, sizeof(float));
  float* res = (float *) calloc(frame_nb, sizeof(float));

  float* out = (float*) calloc((n+no)/frame_size, sizeof(float));
  memset(pulled_data, 0, no);
  memset(overlap, 0, no);
  
  int total_read=0;
  int total_read2=0;

  float error;

  while(1){
    total_read=lpcsrc.pull_data_overlap(n, no, pulled_data, overlap);
    total_read2=exsrc.pull_data_overlap(n, no, pulled_data2, overlap2);
    if(total_read == 0 || total_read2 == 0){
      break;
    }
    // if(total_read < n+no){
    //   std::cout << "ZBRAAAA" << std::endl;
    //   memset((uint8_t*) pulled_data+no+total_read, 0, n+2*no-total_read);
    // }
      
    // if(total_read2 < n+no)
    //   memset((uint8_t*) pulled_data2+no+total_read2, 0, n+2*no-total_read2);

    raw_to_flt(pulled_data, flt_data, frame_nb);
    diff_vec(flt_data, diff, frame_nb);
    raw_to_flt(pulled_data2, res, frame_nb);
    float energy = lpc_filter(diff, coefs, frame_nb-1, lpc_order, &error);

    for(int i=0;i<lpc_order+1;i++){
      std::cout << coefs[i] << " ";
    }
    std::cout << std::endl;
    memcpy(synthetized, res, lpc_order*sizeof(float));

    synthesis_filter(res+lpc_order, coefs, frame_nb-lpc_order, lpc_order, synthetized+lpc_order);

    merge_frames(out, synthetized, n, no, frame_size, merge_overlap);
    // scale(out, energy, (n+no)/frame_size);
    flt_to_raw(out, to_write, (n+no)/frame_size);
    writer.write((uint8_t *) to_write, (n+no));
  }

  free(flt_data);
  free(synthetized);
  free(pulled_data);
  free(pulled_data2);
  free(overlap);
  free(overlap2);
  free(out);
  free(merge_overlap);
  free(res);
  free(to_write);
  free(diff);
}




int main(int argc, char *argv[]){
	OpenOptions op;
  OpenOptions op2;
  po::options_description desc("Allowed options");
  std::string out_filename;
  std::string src_file;
  std::string ex_file;
  desc.add_options()
      ("help", "produces help message")
    	("input,i", po::value(&op.filename), "the names of the input files")
  		("max", po::value(&op.max_time), "the maximum number of seconds to take into account")
    	("skip", po::value(&op.skip_time), "number of seconds to skip at the beginning")
      ("rate", po::value(&op.sample_rate), "sample rate to use")
      ("ex", po::value(&ex_file), "File to be used as the excitation of the lpc filter")
      ("src", po::value(&src_file), "Source file used to compute the lpc coefficients")
      ("out,o", po::value(&out_filename), "Name of the file the write")
    ;

  po::positional_options_description p;
  p.add("input", -1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
  po::notify(vm);
  
  // SndSource src(&op);
  // OutFormat format;
  // format.sample_rate=float(src.get_sample_rate());
  // format.channels=1;
  // format.channel_layout=AV_CH_LAYOUT_MONO;
  // format.out_codec=AV_CODEC_ID_MP3;
  // format.bitrate = 64000;
  // format.sample_fmt= AV_SAMPLE_FMT_S16P;
  // SndWriter writer(out_filename, &format);
  // writer.open();

  // // read_data(src, writer);
  // read_data_overlap(src, writer, 0.02, 0.005f);
  // writer.close();

  options_copy(&op, &op2);

  op.filename=src_file;
	SndSource src(&op);
  op2.filename=ex_file;
  SndSource ex(&op2);
  std::cout << src.get_sample_rate() << std::endl;
  std::cout << ex.get_sample_rate() << std::endl;
  if(src.get_sample_rate() != ex.get_sample_rate()){
    std::cerr << "[ERROR] sample rates of inputs should match" << std::endl;
    exit(1);
  }

  OutFormat format;
  format.sample_rate=float(src.get_sample_rate());
  format.channels=1;
  format.channel_layout=AV_CH_LAYOUT_MONO;
  format.out_codec=AV_CODEC_ID_MP3;
  format.bitrate = 64000;
  format.sample_fmt= AV_SAMPLE_FMT_S16P;
  print_out_format(format);
  SndWriter writer(out_filename, &format);
  print_out_format(format);
  writer.open();
  lpc_filter(src, ex, writer, 0.02f, 0.005f, 10);
  writer.close();

	return 0;
}