#include <stdio.h>
#include <pulse/simple.h>
#include <stdlib.h>
#include <cblas.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include "vector.h"

#define dtype int16_t

//gcc -o run real.c -lpulse-simple -lblas64 -lm -ldl -funroll-loops -O2

static float *FFTS;
static float *FFTC;
static dtype *sample;
static pa_simple *pa_server = NULL;

static int started = 0;
static vector *song;

const int RATE = 44100;
const int SAMPLE_LEN = 4096; 


int setup_matrices() { //technically this can be faster, but it doesn't need to be
	FFTS = (float *) malloc((SAMPLE_LEN) * (120) * sizeof(float));
	FFTC = (float *) malloc((SAMPLE_LEN) * (120) * sizeof(float));
	
	if (FFTS == NULL || FFTC == NULL) {
		printf("Could not allocate memory for Fourier matrices\n");
		return -1;
	}
	for (int i = 0; i < 120; i++) {
		for (int j = 0; j < SAMPLE_LEN; j++) {
			float fi = (float)(i);
			float freq = (27.5 * pow(2.0, fi/12.0)) * (((float) j)/((float) RATE)) * 6.283185307179586;
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
	pa_server = (pa_simple *) pa_simple_new(NULL, "real", PA_STREAM_RECORD, NULL, "record", &spec, NULL, NULL, NULL);
	
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
	printf("%s\n", err);
	pa_simple_free(pa_server);
	free(FFTS);
	free(FFTC);
	free(sample);
	free_vector(song);
	return 0;
}

int extract() {
	float *note = (float *) calloc(12, sizeof(float)); // need this to be persistent
	float fft_s [120];
	float fft_c [120];
	float X [SAMPLE_LEN];

	if (started != 1 || note == NULL) {
		return -1;
	}

	for (int i = 0; i < SAMPLE_LEN; i++) {
		X[i] = (float)(sample[i]);
	}

	// http://www.netlib.org/lapack/explore-html/d5/df1/cblas__sgemv_8c.html (this takes the majority of the time)
	cblas_sgemv (CblasRowMajor, CblasNoTrans, 120, SAMPLE_LEN, 1.0, FFTS, SAMPLE_LEN, X, 1, 0.0, fft_s, 1);
	cblas_sgemv (CblasRowMajor, CblasNoTrans, 120, SAMPLE_LEN, 1.0, FFTC, SAMPLE_LEN, X, 1, 0.0, fft_c, 1);
	
	for (int i = 0; i < 120; i++) {
		float s = fft_s[i];
		float c = fft_c[i];
		note[i%12] += s*s + c*c;
	}

	float norm = 1/cblas_snrm2(12, note, 1);
	cblas_sscal(12, norm, note, 1);

	return append(song, note);
}

void write_sample() {
	FILE *fp;
	fp = fopen("samples.csv", "w");
	for (int i = 0; i < SAMPLE_LEN; i++) {
		fprintf(fp, "%d\n", sample[i]);
	}
	fclose(fp);
}

void write_note_arr_csv(const char *fname) {
	FILE *fp;
	fp = fopen(fname, "w");
	for (int i = 0; i < size(song); i++) {
		for (int j = 0; j < 11; j++) {
			fprintf(fp, "%.4f, ", get(song, i)[j]);
		}
		fprintf(fp, "%.4f\n", get(song, i)[11]);
	}
	fclose(fp);
}

void display_note(int idx) { // TODO: make this more useful
	for (int i = 0; i < 12; i++){
		printf("%f\n", get(song, idx)[i]);
	}
	printf("\n");
}

void display_last_note() {
	display_note(size(song) - 1);
}

int main(int argc, char *argv[]) {
	int record_seconds = 4;
	if (argc > 1) {
		record_seconds = atoi(argv[1]);
		if (record_seconds <= 0) {
			return teardown("invalid number of seconds to record");
		} 
	}

	if (setup() == -1) {
		return teardown("setup failed");
	}

	struct timeval start, end;
	for (int num_samples = 0; num_samples < ((record_seconds * RATE)/SAMPLE_LEN) + 1; num_samples++) {
		if (pa_simple_read(pa_server, (void *)sample, SAMPLE_LEN * sizeof(dtype), NULL) < 0) {
			return teardown("reading from pulseaudio failed");
		}

		gettimeofday(&start, NULL);
			int e = extract();
		gettimeofday(&end, NULL);
		printf("%d: took %lu us\n", num_samples, ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec));

		if (e == -1)
			return teardown("extract failed");
		display_last_note();
	}

	write_sample(); // just for testing
	write_note_arr_csv("tune.csv");


	return teardown("success");
}
