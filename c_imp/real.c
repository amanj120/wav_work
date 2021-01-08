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
const int num_notes = 96; //8 octaves: A1 through Ab8
const float start_freq = 55.0;

static const char *NOTES[12] = {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};

static int verbose;
static float sensitivity;

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
	printf("%s", err);
	if (pa_server) pa_simple_free(pa_server);
	if (FFTS) free(FFTS);
	if (FFTC) free(FFTC);
	if (sample) free(sample);
	if (song) free_vector(song);
	return 0; //we exit after teardown
}

int extract() {
	float *note = (float *) calloc(12, sizeof(float)); // need this to be persistent
	float fft_s [num_notes];
	float fft_c [num_notes];
	float X [SAMPLE_LEN];

	if (started != 1 || note == NULL) {
		return -1;
	}

	for (int i = 0; i < SAMPLE_LEN; i++) {
		X[i] = (float)(sample[i]);
	}

	// http://www.netlib.org/lapack/explore-html/d5/df1/cblas__sgemv_8c.html (this takes the majority of the time)
	cblas_sgemv (CblasRowMajor, CblasNoTrans, num_notes, SAMPLE_LEN, 1.0, FFTS, SAMPLE_LEN, X, 1, 0.0, fft_s, 1);
	cblas_sgemv (CblasRowMajor, CblasNoTrans, num_notes, SAMPLE_LEN, 1.0, FFTC, SAMPLE_LEN, X, 1, 0.0, fft_c, 1);
	
	for (int i = 0; i < num_notes; i++) {
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
	float *note = get(song, idx);

	float sum = cblas_sasum(12, note, 1);
	int max_idx = cblas_isamax(12, note, 1);
	float max = note[max_idx];
	float strength = max/sum;

	if (strength > sensitivity) {
		printf("sample %d:\t%s\t(strength: %.3f)\n",idx, NOTES[max_idx], strength);
	} else {
		printf("sample %d:\tnoise\t(strength: %3f)\n", idx, strength);
	}

	if (verbose) {
		printf("    note strengths: [");
		for (int i = 0; i < 11; i++){
			printf("%s: %.3f, ", NOTES[i], note[i]);
		}
		printf("%s: %.3f]\n", NOTES[11], note[11]);
	}
}

void display_last_note() {
	display_note(size(song) - 1);
}

void update_recording(int cur, int len) {
	int bar_length = 40;
	int bars = (bar_length * (cur+1))/len;
	printf("\rrecording: 	");
	for (int i = 0; i < bars; i++) {
		printf("#");
	}
	for (int i = bars; i < bar_length; i++) {
		printf(".");
	}
	fflush(stdout);
}

const char *menu =
"Welcome to my music transcribing tool\n"
"Options:\n"
"-h, --help\t\t: print this help menu\n"
"-t, --time\t\t: amount of time to record for, default is 2 seconds, valid values are [1,300]\n"
"-v, --verbose\t\t: print all the note strengths\n"
"-s, --sensitivity\t: how sensitive the program is to noise\n"
"\t\t\t  * if (max(note_strengths)/sum(note_strengths) <= sensitivity) {\n"
"\t\t\t  * \t//this audio sample is noise\n"
"\t\t\t  * }\n"
"\t\t\t  * valid values are [0,100] (percent)\n\n";


int main(int argc, char *argv[]) {
	printf("\n");
	int record_seconds = 2;
	verbose = 0;
	sensitivity = 0.5;

	for (int input = 1; input < argc; input++) {
		char *option = argv[input];
		if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
			return teardown(menu);
		} else if (strcmp(option, "-t") == 0 || strcmp(option, "--time") == 0) {
			char *value = argv[++input];
			int t = atoi(value);
			if (t <= 0 || t > 300) {
				return teardown("invalid argument for --time (-t) : expect a positive integer in the range [1,300]");
			} else {
				record_seconds = t;
			}
		} else if (strcmp(option, "-v") == 0 || strcmp(option, "--verbose") == 0) {
			verbose = 1;			
		} else if (strcmp(option, "-s") == 0 || strcmp(option, "--sensitivity") == 0) {
			char *value = argv[++input];
			int t = atoi(value);
			if (t < 0 || t > 100) {
				return teardown("invalid argument for --sensitivity (-s) : expect a positive integer in the range [0,100]");
			} else {
				sensitivity = ((float)t)/100.0;
			}
		} else {
			return teardown("invalid input parameter\n");
		}
	}

	if (setup() == -1)
		return teardown("setup failed\n");

	struct timeval start, end;
	int num_loops = ((record_seconds * RATE)/SAMPLE_LEN) + 1;
	for (int loop = 0; loop < num_loops; loop++) {
		if (pa_simple_read(pa_server, (void *)sample, SAMPLE_LEN * sizeof(dtype), NULL) < 0)
			return teardown("reading from pulseaudio failed\n");
		if (extract() == -1)
			return teardown("extract failed\n");
		update_recording(loop, num_loops);
	}

	printf("\n");
	display_last_note();
	write_sample();
	write_note_arr_csv("tune.csv");
	
	return teardown("");
}


/*		//insert in for loop
		gettimeofday(&start, NULL);
		int e = extract();	
		gettimeofday(&end, NULL);
		printf("%d: took %lu us\n", num_samples, ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec));

		if (e == -1)
			return teardown("extract failed");
		*/