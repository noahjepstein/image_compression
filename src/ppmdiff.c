//filename: ppmdiff.c
//testing file for compressed/decompressed ppm files
//compares two ppm files to find differences between the two. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"


void compare_files(FILE *orig_fp, FILE *new_fp);

int main(int argc, char* argv[]) 
{
	if (argc != 3) {
	    fprintf(stderr, "Usage: ./%s [orig_file.ppm] [file_to_test.ppm]\n", argv[0]);
	    exit(1);
	} 
	
	FILE *orig_fp;
	FILE *new_fp;
	
	if (strcmp(argv[1], "-")) {
		orig_fp = stdin;
	}
	else {
		orig_fp = fopen(argv[1], "r");
	}
	
	if (strcmp(argv[2], "-")) {
		new_fp = stdin;
	}
	else {
		new_fp = fopen(argv[2], "r");
	}
	
	assert(orig_fp != NULL);
	assert(new_fp != NULL);
	compare_files(orig_fp, new_fp);
	return 0;
}

void compare_files(FILE *orig_fp, FILE *new_fp)
{
	A2Methods_T method = uarray2_methods_plain;
	assert(method);
	
	Pnm_ppm orig = Pnm_ppmread(orig_fp, method);
	Pnm_ppm test = Pnm_ppmread(new_fp, method);
	
	unsigned int w_diff = orig->width - test->width;
	unsigned int h_diff = orig->height - test->height;
	
	if (w_diff > 1) {
	      fprintf(stderr, "%s\n", "Image widths do not match.");
	      fprintf(stdout, "%s\n", "1.0");
	}
	if (h_diff > 1) {
	      fprintf(stderr, "%s\n", "Image heights do not match.");
	      fprintf(stdout, "%s\n", "1.0");
	}
}


