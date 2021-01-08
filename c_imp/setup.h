#ifndef SETUP_H_GUARD
#define SETUP_H_GUARD

#define dtype int16_t

extern float *FFTS;
extern float *FFTC;
extern dtype *sample;
extern pa_simple *pa_server;

static int started = 0;
extern vector *song;

extern const int RATE;
extern const int SAMPLE_LEN; 
extern const int num_notes;
extern const float start_freq;

int setup_matrices();
int setup_pa_server();
int setup_sample();
int setup();
int teardown(const char *err);
int setup_complete();

#endif
