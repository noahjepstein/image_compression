/* Filename:         packpix.h
 * Last Modified:    Mar 1st, 2014
 *
 * Acknowledgements: See README.txt
 *
 * Description:      Header file for PACKPIX module.
 */

#include <stdlib.h>
#include <stdio.h>
#include "uarray2.h"
#include "uarray2b.h"

UArray2_T comp_vid_to_word(UArray2b_T cv_array);

UArray2b_T word_to_comp_vid(UArray2_T word_array);
