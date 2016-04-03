#ifndef _WINDOW_H_
#define _WINDOW_H_
#include <cmath>
#include <stdint.h>


#define WINDOW_RECT        0x01
#define WINDOW_BLACKMAN    0x02
#define WINDOW_HANNING     0x04
#define WINDOW_HAMMING     0x08

inline void _apply_cosine_window(double* data, int len, double a0, double a1, double a2, double a3){
    for (int i = 0; i < len; ++i) {
        data[i] *= (a0
                    - a1 * cos((2 * M_PI * i) / len)
                    + a2 * cos((4 * M_PI * i) / len)
                    - a3 * cos((6 * M_PI * i) / len));
    }
}

inline void _apply_cosine_window_idx(double* data, int len, double a0, double a1, double a2, double a3, int i){
        *data *= (a0
                    - a1 * cos((2 * M_PI * i) / len)
                    + a2 * cos((4 * M_PI * i) / len)
                    - a3 * cos((6 * M_PI * i) / len));
}

inline void _apply_cosine_windowf(float* data, int len, double a0, double a1, double a2, double a3){
    for (int i = 0; i < len; ++i) {
        data[i] *= float(a0
                    - a1 * cos((2 * M_PI * i) / len)
                    + a2 * cos((4 * M_PI * i) / len)
                    - a3 * cos((6 * M_PI * i) / len));
    }
}

inline void _apply_cosine_window_idxf(float* data, int len, double a0, double a1, double a2, double a3, int i){
        *data *= float(a0
                    - a1 * cos((2 * M_PI * i) / len)
                    + a2 * cos((4 * M_PI * i) / len)
                    - a3 * cos((6 * M_PI * i) / len));
}


inline void apply_window(double* data, int len, int wind){
	switch (wind){
		case WINDOW_RECT:
			return;
		case WINDOW_BLACKMAN:
			_apply_cosine_window(data, len, 0.42, 0.50, 0.08, 0.0);
			break;
		case WINDOW_HANNING:
			_apply_cosine_window(data, len, 0.50, 0.50, 0.0, 0.0);
			break;
		case WINDOW_HAMMING:
			_apply_cosine_window(data, len, 0.54, 0.46, 0.0, 0.0);
			break;
	}
}

inline void apply_window_idx(double* data, int len, int wind, int i){
	switch (wind){
		case WINDOW_RECT:
			return;
		case WINDOW_BLACKMAN:
			_apply_cosine_window_idx(data, len, 0.42, 0.50, 0.08, 0.0, i);
			break;
		case WINDOW_HANNING:
			_apply_cosine_window_idx(data, len, 0.50, 0.50, 0.0, 0.0, i);
			break;
		case WINDOW_HAMMING:
			_apply_cosine_window_idx(data, len, 0.54, 0.46, 0.0, 0.0, i);
			break;
	}
}



inline void apply_windowf(float* data, int len, int wind){
	switch (wind){
		case WINDOW_RECT:
			return;
		case WINDOW_BLACKMAN:
			_apply_cosine_windowf(data, len, 0.42, 0.50, 0.08, 0.0);
			break;
		case WINDOW_HANNING:
			_apply_cosine_windowf(data, len, 0.50, 0.50, 0.0, 0.0);
			break;
		case WINDOW_HAMMING:
			_apply_cosine_windowf(data, len, 0.54, 0.46, 0.0, 0.0);
			break;
	}
}

inline void apply_window_idxf(float* data, int len, int wind, int i){
	switch (wind){
		case WINDOW_RECT:
			return;
		case WINDOW_BLACKMAN:
			_apply_cosine_window_idxf(data, len, 0.42, 0.50, 0.08, 0.0, i);
			break;
		case WINDOW_HANNING:
			_apply_cosine_window_idxf(data, len, 0.50, 0.50, 0.0, 0.0, i);
			break;
		case WINDOW_HAMMING:
			_apply_cosine_window_idxf(data, len, 0.54, 0.46, 0.0, 0.0, i);
			break;
	}
}

#endif