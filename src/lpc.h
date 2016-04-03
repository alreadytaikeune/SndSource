#ifndef _LPC_H_
#define _LPC_H_

#define LPC_MAX_ORDER 100

#define LPC_MAX_N 16384		/* maximum no. of samples in frame */


typedef struct lpc_result{
	float k[LPC_MAX_ORDER+1];
	float nsigma;
	float coefs[LPC_MAX_ORDER+1];
} lpc_result;

void lpc_filter(
  float Sn[],	/* Nsam samples with order sample memory */
  float a[],	/* order+1 LPCs with first coeff 1.0 */
  int Nsam,	/* number of input speech samples */
  int order,	/* order of the LPC analysis */
  float *E	/* residual energy */
);

void autocorrelate(
  float Sn[],	/* frame of Nsam windowed speech samples */
  float Rn[],	/* array of P+1 autocorrelation coefficients */
  int Nsam,	/* number of windowed samples to use */
  int order	/* order of LPC analysis */
);


void levinson_durbin(
  float R[],		/* order+1 autocorrelation coeff */
  float lpcs[],		/* order+1 LPC's */
  int order		/* order of the LPC analysis */
);

void inverse_filter(
  float Sn[],	/* Nsam input samples */
  float a[],	/* LPCs for this frame of samples */
  int Nsam,	/* number of samples */
  float res[],	/* Nsam residual samples */
  int order	/* order of LPC */
);

void synthesis_filter(
  float res[],	/* Nsam input residual (excitation) samples */
  float a[],	/* LPCs for this frame of speech samples */
  int Nsam,	/* number of speech samples */
  int order,	/* LPC order */
  float Sn_[]	/* Nsam output synthesised speech samples */
);

void hanning_window(
  float Sn[], /* input frame of speech samples */
  float Wn[], /* output frame of windowed samples */
  int Nsam  /* number of samples */
);

#endif