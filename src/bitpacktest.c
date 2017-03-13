#include <stdlib.h>
#include <stdio.h>
#include <bitpack.h>
#include <time.h>
#include <assert.h>
#include <math.h>

void check_laws(uint64_t word, unsigned width, unsigned width2, unsigned lsb,
							unsigned lsb2);

void check_manual();

int main(int argc, char *argv[]) 
{
	(void)argc;
	(void)argv;
	FILE *randfp = fopen("/dev/urandom", "rb");
	assert(randfp != NULL);

	uint64_t word;
	size_t rc = fread(&word, sizeof(word), 1, randfp);
	assert(rc == 1);
	int w2;
	int lsb2;
	srand(time(NULL));

	for (unsigned w = 0; w <= 64; w++) {
		for (unsigned lsb = 0; lsb + w < 64; lsb++) {
			for (unsigned trial = 0; trial < 10; trial ++) {
				
				w2   = rand() % 64;
				lsb2 = rand() % 64; 
				
				while(w2 + lsb2 >= 64) {
					w2   = rand() % 64;
					lsb2 = rand() % 64;	
				}
				check_laws(word, w, w2, lsb, lsb2);
			}
		}
	}
	// check_manual();
	fprintf(stderr, "%s\n", "Passed.");
}

void check_laws(uint64_t word, unsigned width, unsigned width2, unsigned lsb,
							unsigned lsb2) 
{ 
	int64_t sval = (int64_t)((uint64_t)rand() % (uint64_t)pow(2, width));
	// uint64_t uval = 0;

	// check bitpack functions

	if (width + lsb < lsb2) {
		assert(Bitpack_gets(Bitpack_news(word, width, lsb, sval), width2, lsb2) 
				       == Bitpack_gets(word, width2, lsb2));
	}

	(void)width2;
	(void)lsb2;
	assert(Bitpack_gets(Bitpack_news(word, width, lsb, sval), width, lsb) == sval);
}

void check_manual()
{
	assert(Bitpack_fitsu(0, 9));
	assert(Bitpack_fitss(0, 9));
	assert(Bitpack_fitss(-256, 9));
	// assert(Bitpack_fitss(-257, 9));
	// assert(Bitpack_fitss(256, 9));
	assert(Bitpack_fitss(255, 9));
	assert(Bitpack_fitss(4, 8));

	assert(Bitpack_gets(-40, 3, 4) == 3);
	uint64_t word = Bitpack_gets(-40, 3, 4);
	// Bitpack_gets(90214533, 14, 32);
	// fprintf(stderr, "%ld\n"Bitpack_gets(-90214533, 5, ))
	fprintf(stderr, "%ld %ld\n", word, Bitpack_newu(0, 4, 4, word));
	assert(Bitpack_getu(Bitpack_newu(pow(2, 63), 4, 4, 1), 4, 4) == 1);
}


