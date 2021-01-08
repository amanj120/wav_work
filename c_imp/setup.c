#include <stdio.h>
#include <pulse/simple.h>
#include <stdlib.h>
#include <math.h>

#include "vector.h"
#include "setup.h"

int setup_complete() {
	return started;
}

int setup_matrices() { //technically this can be faster, but it doesn't need to be
	FFTS = (float *) malloc((SAMPLE_LEN) * (num_notes) * sizeof(float));
	FFTC = (float *) malloc((SAMPLE_LEN) * (num_notes) * sizeof(float));
	
	if (FFTS == NULL || FFTC == NULL) {
		printf("Could not allocate memory for Fourier matrices\n");
		return -1;
	}
	for (int i = 0; i < num_notes; i++) {
		for (int j = 0; j < SAMPLE_LEN; j++) {
			float fi = (float)(i);
			float freq = (start_freq * pow(2.0, fi/12.0)) * (((float) j)/((float) RATE)) * 6.283185307179586;
			FFTS[i*SAMPLE_LEN + j] = (float) sin(freq);
			FFTC[i*SAMPLE_LEN + j] = (float) cos(freq);
		}
	}
	return 1;
}

int setup_pa_server() {
	pa_sample_spec spec;
	spec.format = PA_SAMPLE_S16NE; // PA_SAMPLE_FLOAT32LE; // ranges from -1.0 to 1.0
	spec.channels = 1;
	spec.rate = RATE;
	 
	// default server, application name, record audio (instead of write), description of stream,
	// sample format, default channel map, default buffering attributes, ignore error codes
	pa_server = (pa_simple *) pa_simple_new(NULL, "app", PA_STREAM_RECORD, NULL, "record", &spec, NULL, NULL, NULL);
	if (pa_server == NULL) {
		return -1;
	} else {
		return 1;
	}
}

int setup_sample() {
	sample = (dtype *) malloc(SAMPLE_LEN * sizeof(dtype));
	if (sample == NULL) {
		return -1;
	} else {
		return 1;
	}
}

//return -1 on failure, 1 otherwise
int setup() {
	if (started == 0) {
		started = 1;
		if (setup_matrices() == -1) {
			printf("setup matrices failed\n");
			return -1;
		} 
		if (setup_pa_server() == -1) {
			printf("setup pulseaudio server failed\n");
			return -1;
		}
		if (setup_sample() == -1) {
			printf("setup sample failed\n");
			return -1;
		}
		song = new_vector();
		if (song == NULL) {
			printf("setup note vector failed\n");
			return -1;
		}
	}
	return 1;
}

int teardown(const char *err) {
	printf("%s", err);
	if (pa_server) pa_simple_free(pa_server);
	if (FFTS) free(FFTS);
	if (FFTC) free(FFTC);
	if (sample) free(sample);
	if (song) free_vector(song);
	return 0; //we exit after teardown
}
