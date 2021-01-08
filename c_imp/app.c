#include "app.h"
#include "setup.h"

int extract() {
	float *note = (float *) calloc(12, sizeof(float)); // need this to be persistent
	float fft_s [num_notes];
	float fft_c [num_notes];
	float X [SAMPLE_LEN];

	if (setup_complete() != 1 || note == NULL) {
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
		printf("sample %d:\tnoise\t(strength: %.3f)\n", idx, strength);
	}

	if (verbose) {
		printf("    note strengths: [");
		for (int i = 0; i < 11; i++){
			printf("%s: %.3f, ", NOTES[i], note[i]);
		}
		printf("%s: %.3f]\n", NOTES[11], note[11]);
	}
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

int main(int argc, char *argv[]) {
	printf("\n");
	int record_seconds = 2;
	verbose = 0;
	sensitivity = 0.5;
	char *fname = "tune.csv";

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
		} else if (strcmp(option, "-o") == 0 || strcmp(option, "--out") == 0) {
			fname = argv[++input];
			int fname_len = strlen(fname);
			if (fname_len <= 4 || strcmp(fname + fname_len - 4, ".csv") != 0) {
				return teardown("invalid argument for --out (-o) : expect a file name ending in '.csv'");
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
	for (int idx = 0; idx < num_loops; idx++) {
		display_note(idx);
	}
	write_sample();
	write_note_arr_csv(fname);
	
	return teardown("");
}
