/* Filename:         40image.c
 * Authors:          Noah Epstein (nepste01), Katie Kurtz (kkurtz01)
 * Last Modified:    Feb 25th, 2014
 *
 * Acknowledgements: Most of this file was given in the HW4 Arith 
 *                   specifications.
 *
 * Description:      Main for 40image pixmap compression program. Handles 
 *                   command line inputs. Takes a file from stdin or as 
 *                   a command-line argument. Prints to stdout. Includes 
 *                   features for compressing a PPM RGB pixmap to a binary
 *                   image format and decompressing from binary format to a
 *                   PPM RGB pixmap. Binary compressed file should be about 
 *                   one-fourth the size of the decompressed PPM image. 
 * 
 *                   Also features a utility called test40 for testing the 
 *                   "tiers" of the image compression. test40 does not print
 *                   a compressed image to stdout, but compresses a PPM and
 *                   decompresses it, printing the resultant PPM to stdout. 
 * 
 * Usage:            To compress:    ./40image -c [infile.ppm] > [outfile.bin]
 *                   To decompress:  ./40image -d [infile.bin] > [outfile.ppm]
 *                   To test:        ./40image -t [infile.ppm] > [outfile.ppm]
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include <compress40.h>

extern void test40(FILE *input);
static void (*compress_or_decompress)(FILE *input) = compress40;

int main(int argc, char *argv[])
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-c") == 0) {
                        compress_or_decompress = compress40;
                } else if (strcmp(argv[i], "-d") == 0) {
                        compress_or_decompress = decompress40;
                } else if (strcmp(argv[i], "-t") == 0) {
                        compress_or_decompress = test40;                        
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n",
                                argv[0], argv[i]);
                        exit(1);
                } else if (argc - i > 2) {
                        fprintf(stderr, "Usage: %s -d [filename]\n"
                                "       %s -c [filename]\n",
                                argv[0], argv[0]);
                        exit(1);
                } else {
                        break;
                }
        }
        assert(argc - i <= 1);    /* at most one file on command line */
        if (i < argc) {
                FILE *fp = fopen(argv[i], "r");
                assert(fp != NULL);
                compress_or_decompress(fp);
                fclose(fp);
        } else {
                compress_or_decompress(stdin);
        }
}
