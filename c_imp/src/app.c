#include "app.h"
#include "setup.h"
#include <ncurses.h>

char* sort_and_print_prior(double *prior) {
	for (int i = 0; i < 12; i++) {
		for (int j = i+1; j < 12; j++) {
			if (prior[i] == prior[j]) {
				prior[i] += 0.0000001;
				i = 0;
			}
		}
	}

	double tosort[12] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	int names[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	for(int i = 0; i < 12; i++) {
		if (prior[i] > tosort[0]) {
			tosort[0] = prior[i];
			names[0] = i; 
		}
	}

	for(int n = 1; n < 12; n++) {
		for(int i = 0; i < 12; i++) {
			if (prior[i] > tosort[n] && prior[i] < tosort[n-1]) {
				tosort[n] = prior[i];
				names[n] = i;
			}
		}
	}

	char * retstr = calloc(200, 1);//overshoot size
	int index = 0;
	for (int i = 0; i < 12; i++) {
		if (tosort[i] > 1.00) //make sure less than 1
			tosort[i] = 1.00;
		char temp[20];
		const char * fmt = "[%s:% .1f%%]";
		sprintf(temp, fmt, NOTES[names[i]], (tosort[i] * 99.9));
		sprintf((retstr +index), fmt, NOTES[names[i]], (tosort[i] * 99.9));
		index += strlen(temp);
	}
	return retstr;
}

char *print_and_update_prior(double * prior) {
	char * retstr = sort_and_print_prior(prior);

	double sum = 0.0;
	float *cur = (float *)tail(song);
	for (int i = 0; i < 12; i++) {
		sum += cur[i];
	}

	// update 1
	// for (int i = 0; i < 12; i++) {
	// 	double p_temp = prior[i];
	// 	double v_temp = (double)cur[i]/sum;
	// 	prior[i] = (p_temp * v_temp) / (p_temp * v_temp + (1-p_temp)*(1-v_temp));
	// }

	//update 2: average
	// double num_samples = (double) size(song);
	double num_samples = 9; //about 1 second
	for (int i = 0; i < 12; i++) {
		prior[i] = (prior[i] * (num_samples - 1) + cur[i])/num_samples;
	}

	sum = 0.0;
	for (int i = 0; i < 12; i++) {
		sum += prior[i];
	}

	for (int i = 0; i < 12; i++) {
		prior[i] /= sum;
	}

	return retstr;
}

int run_interactive(char *fname) {
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	
	int ch = 'n';
	FILE *fp;
	fp = fopen(fname, "w");
	double *prior = (double *) malloc(12 * sizeof(double));
	int paused = 0;
	char * str = NULL;
	vector *dels = new_vector();
	int leave = 1;

	while (leave) {		
		if (ch == 'n') {
			paused = ~paused;
			printw("\n to record the next note, press 'n', to exit, press 'q'\r");
			clear_vector(song);
			for (int i = 0; i < 12; i++) {
				prior[i] = 1.0/12.0;
			}
			if (str != NULL & paused != 0) {
				fprintf(fp, "%s\n", str);
			}
		} 
		if (paused == 0) {
			if (pa_simple_read(pa_server, (void *)sample, SAMPLE_LEN * sizeof(dtype), NULL) < 0){
				endwin();
				fclose(fp);
				return -1; 
			}
			if (extract() == -1) {
				endwin();
				fclose(fp);
				return -1;
			}
			str = print_and_update_prior(prior);
			printw("\r\t%s", str);
			append(dels, str);
		} else if (ch == 'q') {
			leave = 0;
		}
		refresh();
		ch = getch();
	}

	free(prior);
	fclose(fp);
	free_vector(dels);
	endwin();
	return 0;
}

int run_uninteractive(int record_seconds, char *fname) {
	struct timeval start, end;
	int num_loops = ((record_seconds * RATE)/SAMPLE_LEN) + 1;
	for (int loop = 0; loop < num_loops; loop++) {
		if (pa_simple_read(pa_server, (void *)sample, SAMPLE_LEN * sizeof(dtype), NULL) < 0) {
			return -1;
		}
		if (extract() == -1){
			return -2;
		}
		update_recording(loop, num_loops);
	}

	printf("\n");
	for (int idx = 0; idx < num_loops; idx++) {
		display_note(idx);
	}
	write_sample();
	write_note_arr_csv(fname);
	return 0;
}


int main(int argc, char *argv[]) {
	int record_seconds = 2;
	verbose = 0;
	sensitivity = 0.5;
	char *fname = "tune.csv";
	int interactive = 0;

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
		} else if (strcmp(option, "-i") == 0 || strcmp(option, "--interactive") == 0) {
			interactive = 1;			
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
			if (fname_len <= 1) {
				return teardown("invalid argument for --out (-o) : expect a non-empty file name");
			}
		} else {
			return teardown("invalid input parameter\n");
		}
	}



	if (setup() == -1) {
		return teardown("setup failed\n");
	}

	if (interactive == 1) {
		if (run_interactive(fname) == -1) {
			return teardown("error encountered running in interactive mode\n");
		}
	} else {
		int r = run_uninteractive(record_seconds, fname);
		if (r == -1) {
			return teardown("reading from pulseaudio failed\n");
		} else if (r == -2) {
			return teardown("extract failed\n");
		}
	}
	
	return teardown("");
}



int extract() {
	float fft_s [num_notes];
	float fft_c [num_notes];
	float X [SAMPLE_LEN];
	
	float *note = (float *) calloc(12, sizeof(float)); // need this to be persistent

	if (setup_complete() != 1 || note == NULL) {
		return -1;
	}

	for (int i = 0; i < SAMPLE_LEN; i++) {
		X[i] = (float)(sample[i]);
	}

	// http://www.netlib.org/lapack/explore-html/d5/df1/cblas__sgemv_8c.html (this is the most time-consuming part)
	cblas_sgemv(CblasRowMajor, CblasNoTrans, num_notes, SAMPLE_LEN, 1.0, FFTS, SAMPLE_LEN, X, 1, 0.0, fft_s, 1);
	cblas_sgemv(CblasRowMajor, CblasNoTrans, num_notes, SAMPLE_LEN, 1.0, FFTC, SAMPLE_LEN, X, 1, 0.0, fft_c, 1);
	
	for (int i = 0; i < num_notes; i++) {
		float s = fft_s[i];
		float c = fft_c[i];
		note[i%12] += s*s + c*c;
	}

	cblas_sscal(12, (1/cblas_snrm2(12, note, 1)), note, 1);
	return append((void *)song, note);
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
		float *notes = (float *)get(song, i); 
		for (int j = 0; j < 11; j++) {
			fprintf(fp, "%.4f, ", notes[j]);
		}
		fprintf(fp, "%.4f\n", notes[11]);
	}
	fclose(fp);
}

void display_note(int idx) { 
// TODO: make the note/noise recognition better 
// (i.e. differentiate between noise, an in tune note, and an out of tune note in between two in tune notes)
	float *note = (float *)get(song, idx);

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