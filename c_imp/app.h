#ifndef APP_H_GUARD
#define APP_H_GUARD

#include <stdio.h>
#include <pulse/simple.h>
#include <stdlib.h>
#include <cblas.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include "vector.h"

#define dtype int16_t

float *FFTS;
float *FFTC;
dtype *sample;
pa_simple *pa_server = NULL;

vector *song;

const int RATE = 44100;
const int SAMPLE_LEN = 4096; 
const int num_notes = 96; //8 octaves: A1 through Ab8
const float start_freq = 55.0;

const char *NOTES[12] = {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};
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
"\t\t\t  * valid values are [0,100] (percent)\n\n"
"-o, --out\t\t: name of output file to write note data to, default is tune.csv";

static int verbose;
static float sensitivity;

int extract();

void write_sample();
void write_note_arr_csv(const char *fname);
void display_note(int idx);
void update_recording(int cur, int len);

#endif
