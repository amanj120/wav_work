#ifndef APP_H_GUARD
#define APP_H_GUARD

#include <stdio.h>
#include <pulse/simple.h>
#include <stdlib.h>
#include <cblas.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <ncurses.h>

#include "vector.h"

#define dtype int16_t

float *FFTS;
float *FFTC;
dtype *sample;
pa_simple *pa_server = NULL;

vector *song;

const int RATE = 44100;
const int SAMPLE_LEN = 4096+2048; 
const int num_notes = 96; //8 octaves: A1 through Ab8
const float start_freq = 55.0;

const char *NOTES[12] = {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};
const char *menu =
"Welcome to my music transcribing tool!\n\n"
"    In the default mode, you can sing/hum/whistle a note\n"
"and press n to move on to record the next note. When you\n"
"are done recording press 'q' to quit and save your file.\n\n"
"Options:\n"
"-h, --help\t\t: print this help menu\n"
"-o, --out\t\t: output file to write to. Default is 'tune.txt'\n"
"-u, --uninteractive\t: record everything all at once, the app will guess notes for you.\n"
"-t, --time\t\t: amount of time to record for. Default is 2 seconds, valid values are [1,300]\n"
"\t\t\t  only works in uninteractive mode\n"
"-v, --verbose\t\t: print all the note strengths\n"
"\t\t\t  only works in uninteractive mode\n"
;

static int verbose;
int extract();

void write_sample();
void write_note_arr_csv(const char *fname);
void display_note(int idx);
void update_recording(int cur, int len);

#endif
