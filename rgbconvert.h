/* Filename:         rgbconvert.h
 * Authors:          Noah Epstein (nepste01), Katie Kurtz (kkurtz01)
 * Last Modified:    Mar 1st, 2014
 *
 * Acknowledgements: See README.txt
 *
 * Description:      Header file for RGBCONVERT module. 
 */

#ifndef RGBCONVERT
#define RGBCONVERT

#include "uarray2b.h"
#include "pnm.h"

extern UArray2b_T rgb_to_comp_vid(Pnm_ppm pixmap);

extern Pnm_ppm comp_vid_to_rgb(UArray2b_T b_img);

#endif
