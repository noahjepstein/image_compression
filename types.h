/* Filename:         types.h
 * Last Modified:    Mar 1st, 2015
 *
 * Acknowledgements: See README.txt
 *
 * Description:      Header file that defines types used in 40image program.
 */

#ifndef TYPES_H
#define TYPES_H

#include "uarray.h"

struct UArray2b_T {
        int width, height;
        unsigned blocksize;
        unsigned size;
        UArray2_T blocks;
};

/* comp_vid contains data about an individual pixel */
struct comp_vid
{
        float lum;
        float pb;
        float pr;
};


/* float comp vid and quant_comp_vid are structs that contain data about a
 * 2x2 block of pixels.
 */
struct float_comp_vid
{
        float a;
        float b;
        float c;
        float d;
        float pb_avg;
        float pr_avg;
};

/* defines quantized bit fields for component video block */
struct quant_comp_vid
{

        unsigned a   : 6;
        unsigned qpb : 4;
        unsigned qpr : 4;
        int      b   : 6;
        int      c   : 6;
        int      d   : 6;
};

/*redifines members in UArray2_T */
struct UArray2_T {
        int width, height;
        int size;
        UArray_T rows;
};

#endif
