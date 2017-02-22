/* Filename:         rgbconvert.c
 * Authors:          Noah Epstein (nepste01), Katie Kurtz (kkurtz01)
 * Last Modified:    Mar 1st, 2014
 *
 * Acknowledgements: See README.txt
 *
 * Description:      RGBConvert is a submodule of 40image that converts all 
 *                   Pnm_rgb pixels stored in a Uarray2b to component video
 *                   pixels. It also converts a Uarray2b of component video 
 *                   pixels to Pnm_rgb pixels. 
 */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uarray2b.h"
#include "rgbconvert.h"
#include "a2methods.h"
#include "a2blocked.h"
#include "compress40.h"
#include "uarray2.h"
#include "types.h"

const unsigned RGB_DENOM = 255;
const int BLOCKSIZE = 2;

void apply_rgb_to_cv_pix(int i, int j, UArray2b_T array, void *pixel, 
                                                                    void *cl);
void apply_cv_to_rgb_pix(int i, int j, UArray2b_T array, void *pixel,
                                                                    void *cl);
Pnm_ppm ppm_from_u2b(UArray2b_T rgb_array);

void clip_rgb(struct Pnm_rgb *pix);


/* Description: Converts a PPM pixmap w/ RGB pixels to UArray2b of component
 *              video pixels. 
 *              
 * Input:       PPM pixmap - the original image to be compressed. 
 * Output:      UArray2b of component-video structs. One tier down. 
 */
UArray2b_T rgb_to_comp_vid(Pnm_ppm pixmap)
{
        Pnm_ppm cv_pixmap = malloc(sizeof(*cv_pixmap));
        cv_pixmap->denominator = pixmap->denominator;

        cv_pixmap->pixels = UArray2b_new(pixmap->width, pixmap->height, 
                                          sizeof(struct comp_vid), BLOCKSIZE);
        
        UArray2b_map(pixmap->pixels, &apply_rgb_to_cv_pix, cv_pixmap);

        UArray2b_T pixels = cv_pixmap->pixels;
        free(cv_pixmap);
        
        return pixels;
}


Pnm_ppm comp_vid_to_rgb(UArray2b_T b_img)
{
        int width = b_img->width;
        int height = b_img->height;
        int size = sizeof(struct Pnm_rgb);
        UArray2b_T rgb_array = UArray2b_new(width, height, size, BLOCKSIZE);
        UArray2b_map(b_img, &apply_cv_to_rgb_pix, rgb_array);

        Pnm_ppm pixmap = ppm_from_u2b(rgb_array);
        return pixmap;
}


/* Description: Makes a new pixmap out of an RGB array 
 *              
 * Input:       PPM pixmap - the original image to be compressed. 
 * Output:      UArray2b of component-video structs. One tier down. 
 */
Pnm_ppm ppm_from_u2b(UArray2b_T rgb_array)
{
        Pnm_ppm pixmap = malloc(sizeof(*pixmap));
        pixmap->width = rgb_array->width;
        pixmap->height = rgb_array->height;
        pixmap->denominator = RGB_DENOM;
        pixmap->pixels = rgb_array;
        pixmap->methods = uarray2_methods_blocked;

        return pixmap;
}


/* Description: Apply function that, when mapped to a Uarray2b of RGB pixels,
 *              turns each RGB pixel into a component-video pixel. 
 *              
 * Input:       Takes i and j indices of the pixel, pointer to the RGB pixel
 *              itself, new target array passed as closure where we'll place
 *              comp_vid structs. 
 * Output:      UArray2b of component-video struct as closure.
 */
void apply_rgb_to_cv_pix(int i, int j, UArray2b_T array, void *pixel, 
                                                                     void *cl)
{
        assert(pixel != NULL);
        Pnm_rgb rgbpix_p = pixel;
        struct Pnm_rgb rgbpix = *rgbpix_p;
        Pnm_ppm cv_pixmap = cl;
        int denom = cv_pixmap->denominator;
        UArray2b_T cv_array = cv_pixmap->pixels;
        struct comp_vid *cvpixel = UArray2b_at(cv_array, i, j);
        assert(cvpixel != NULL);

        /* many calculation for rgb to component video */
        cvpixel->lum = (0.299 * rgbpix.red) + (0.587 * rgbpix.green)  
                                                      + (0.114 * rgbpix.blue);
        cvpixel->pb = -(0.168736 * rgbpix.red) - (0.331264 * rgbpix.green) 
                                                        + (0.5 * rgbpix.blue);
        cvpixel->pr = (0.5 * rgbpix.red) - (0.418688 * rgbpix.green) 
                                                   - (0.081312 * rgbpix.blue);

        //ambiguate denominator so we can assume 255 on decompression
        cvpixel->lum = cvpixel->lum / denom;
        cvpixel->pb = cvpixel->pb / denom;
        cvpixel->pr = cvpixel->pr / denom;
        (void) array;
}


/* Description: Apply function that, when mapped to a Uarray2b of component
 *              video pixels, turns each cv pixel into an RGB pixel. 
 *              
 * Input:       Takes i and j indices of the pixel, pointer to the CV pixel
 *              itself, new target array passed as closure where we'll place
 *              RGB structs. 
 * Output:      UArray2b of RGB structs as closure.
 */
void apply_cv_to_rgb_pix(int i, int j, UArray2b_T array, void *pixel, 
                                                                     void *cl)
{
        (void)array;
        assert(pixel != NULL);
        struct comp_vid *cvpix = pixel;
        UArray2b_T rgb_array = cl;

        assert(cvpix != NULL);

        cvpix->lum = cvpix->lum * RGB_DENOM;
        cvpix->pb = cvpix->pb * RGB_DENOM;
        cvpix->pr = cvpix->pr * RGB_DENOM;      
        
        /* many calculations to go from component video to rgb */
        struct Pnm_rgb *elem = UArray2b_at(rgb_array, i, j);
        signed r = (1.0 * cvpix->lum) + (1.402 * cvpix->pr);
        signed g = (1.0 * cvpix->lum) - (0.344136 * cvpix->pb) 
                                                     - (0.714136 * cvpix->pr);
        signed b = (1.0 * cvpix->lum) + (1.772 * cvpix->pb);

        //limit values while still signed to prevent negative signed values
        //rolling back to extremely large unsigned values.
        if (r < 0)
                r = 0;
        if (b < 0)
                b = 0;
        if (g < 0)
                g = 0;

        elem->red   = r;
        elem->green = g;
        elem->blue  = b;

        clip_rgb(elem);
}


/* Description: Limits values of a pixel to values between 0 and RGB_DENOM
 *              (255). 
 *              
 * Input:       RGB pixel pointer.
 * Output:      Nothing.
 */
void clip_rgb(struct Pnm_rgb *pix)
{

        if (pix->red > RGB_DENOM) {
                pix->red = RGB_DENOM;
        }

        if (pix->green > RGB_DENOM) {
                pix->green = RGB_DENOM;
        }

        if (pix->blue > RGB_DENOM) {
                pix->blue = RGB_DENOM;
        }
}



