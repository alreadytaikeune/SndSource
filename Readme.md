# Dependencies

## FFMPEG libraries
- lavutil
- lavformat
- lavcodec
- lswscale
- lswresample

## Miscellaneous libraries
* lpthread
* lfftw3
* lboost_program_options

# Install

Just run:

```
make build

make
```

# Build libsndsource

```
make lib
```

# Make python wrapper and everything else
```
make all
```

# Examples
## Basic way to read data:

```c
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
```

## Example of reading overlaping data :

```c
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
    // DO THINGS
  }

  free(flt_data);
  free(pulled_data);
  free(overlap);
  free(out);
  free(merge_overlap);

}
```

## Using the wrapper
See test.py for examples of how to use the wrapper.
