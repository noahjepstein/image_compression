/* Filename:         compress40.c
 * Last Modified:    Mar 1st, 2015
 *
 * Acknowledgements: See README.txt
 *
 * Description:      Module for 40image compression program that handles file
 *                   I/O and calls the "tiers" of the image compression and
 *                   decompression in sequence.
 */


#include <compress40.h>
#include <pnm.h>
#include <a2methods.h>
#include "uarray2.h"
#include "a2blocked.h"
#include "rgbconvert.h"
#include "packpix.h"
#include <stdlib.h>
#include "assert.h"
#include "types.h"
#include <bitpack.h>

static A2Methods_T methods;
const int W_SIZE = 8;

Pnm_ppm make_ppm(FILE *input);
UArray2_T make_binary_img(FILE *input);
Pnm_ppm trim(Pnm_ppm img);
void print_compressed(UArray2_T comp_image);
void bitprint(int i, int j, UArray2_T arr, void *elem, void *cl);
void test40(FILE *input);

/*==========================================================================*/

/* Description: ---TEST FUNCTION FOR 40IMAGE---
 *              Takes in a PPM file, stores it as an image, and compresses it:
 *              turning RGB pixels to component video pixels, then turning
 *              component video pixels to bitpacked words.
 *              Decompresses bitpacked words via the reverse of the above.

 * Input:       PPM file pointer. CRE to pass NULL input.
 * Output:      Nothing. Calls functions to print a pixmap to stdout.
 */
void test40(FILE *input)
{
        //compress
        Pnm_ppm img = make_ppm(input);
        assert(img != NULL);
        img = trim(img);
        UArray2b_T comp_vid = rgb_to_comp_vid(img);
        UArray2_T packed_pix = comp_vid_to_word(comp_vid);

        //decompress
        UArray2b_T cvarray = word_to_comp_vid(packed_pix);
        Pnm_ppm pixmap =  comp_vid_to_rgb(cvarray);
        Pnm_ppmwrite(stdout, pixmap);

        Pnm_ppmfree(&img);
        UArray2b_free(&comp_vid);
        UArray2_free(&packed_pix);
        UArray2b_free(&cvarray);
        Pnm_ppmfree(&pixmap);

        (void)comp_vid;
        (void)pixmap;
}


/* Description: Takes in a PPM file, stores it as an image, and compresses it:
 *              turning RGB pixels to component video pixels, then turning
 *              component video pixels to bitpacked words. Prints the
 *              bitpacked words to stdout as a binary file.
 *
 * Input:       PPM file pointer. CRE to pass NULL input.
 * Output:      Nothing. Calls functions to print a binary image to stdout.
 */
void compress40  (FILE *input)
{
        Pnm_ppm img = make_ppm(input);
        assert(img != NULL);
        img = trim(img);
        UArray2b_T comp_vid = rgb_to_comp_vid(img);
        UArray2_T packed_pix = comp_vid_to_word(comp_vid);
        print_compressed(packed_pix);

        Pnm_ppmfree(&img);
        UArray2b_free(&comp_vid);
        UArray2_free(&packed_pix);
}


/* Description: Takes in a binary compressed file, stores, and compresses it.
 *              Allocates memory to store the binary file, turns bitpacked
 *              words into component video pixels, turns component video
 *              pixels into RGB pixels, then prints as a PPM file.
 *
 * Input:       Binary compressed image file pointer. CRE to pass NULL input.
 * Output:      Nothing. Calls functions to write a PPM to stdout.
 */
void decompress40(FILE *input)
{
        assert(input != NULL);
        methods = uarray2_methods_blocked;
        UArray2_T bimg = make_binary_img(input);
        UArray2b_T cvarray = word_to_comp_vid(bimg);
        Pnm_ppm pixmap =  comp_vid_to_rgb(cvarray);
        Pnm_ppmwrite(stdout, pixmap);

        UArray2_free(&bimg);
        UArray2b_free(&cvarray);
        Pnm_ppmfree(&pixmap);
}


/* Description: Takes in a ppm file pointer and reads to create a PPM.
 *
 * Input:       PPM file pointer. CRE for null input.
 * Output:      PPM_PPM image.
 */
Pnm_ppm make_ppm(FILE *input)
{
        assert(input != NULL);
        methods = uarray2_methods_blocked;
        Pnm_ppm pix = Pnm_ppmread(input, methods);
        return pix;
}

/* Description: Takes in a binary compressed file, reads its header, and
 *              stores the image data in a 2D array of 32-bit words.
 *
 * Input:       Binary compressed image file pointer. CRE to pass NULL input.
 * Output:      UArray2_t that holds bitpacked iamge data.
 */
UArray2_T make_binary_img(FILE *input)
{
        assert(input != NULL);

        unsigned height, width;
        int read = fscanf(input, "COMP40 Compressed image format 2\n%u %u",
                                                       &width, &height);
        assert(read == 2);
        int c = getc(input);
        assert (c == '\n');
        uint64_t word;

        UArray2_T binary_img_array = UArray2_new(width, height, W_SIZE);
        c =  getc(input);

        for (unsigned int i = 0; i < height; i ++){
                for (unsigned int j = 0; j < width; j++){
                        //loops through word, putting bytes in big-endian
                        for (unsigned k = 0; k < 4; k++) {

                                word  = Bitpack_newu(word, W_SIZE,
                                                     k * W_SIZE, (unsigned)c);
                                c =  getc(input);
                        }

                        uint32_t *elem = UArray2_at(binary_img_array, j, i);
                        *elem = word;
                }
        }
        return binary_img_array;
}


/* Description: Trims a pnm_ppm so that it has even width and height values.
 *              Does nothing for even widths and heights. Creates a new image
 *              for odd widths and heights and puts the old image less the
 *              odd row or column into the new image.
 *
 * Input:       PPM image. CRE to pass NULL input.
 * Output:      Pmn_ppm - original for even width and height, new Pnm_ppm for
 *              odd width or height.
 */
Pnm_ppm trim(Pnm_ppm img)
{
        assert(img != NULL);
        unsigned int widthnew = img->width;
        unsigned int heightnew = img->height;

        if(widthnew % 2 != 0)
                widthnew --;
        if (heightnew % 2 != 0)
                heightnew --;

        if (widthnew != img->width || heightnew != img->height) {

                UArray2b_T newarray = UArray2b_new(widthnew, heightnew,
                                                        sizeof(UArray2_T), 2);
                for (unsigned int i = 0; i < heightnew; i++){

                        for (unsigned int j = 0; j < widthnew; j++){

                                struct Pnm_rgb *origelem
                                           = UArray2b_at( img->pixels, j, i );
                                struct Pnm_rgb *newelem
                                           = UArray2b_at( newarray, j, i );

                                newelem->red = origelem->red;
                                newelem->green = origelem->green;
                                newelem->blue = origelem->blue;
                        }
                }

                img->width = widthnew;
                img->height = heightnew;
                img->pixels = newarray;
        }
        return img;
}


/* Description: Prints a binary image that consists of 32-bit bitpacked image
 *              data. Uses below header format.
 *
 * Input:       UArray2 of bitpacked image data.
 * Output:      None. Prints to stdout.
 */
void print_compressed(UArray2_T comp_image)
{
        fprintf(stdout, "COMP40 Compressed image format 2\n%u %u\n",
                               comp_image->width, comp_image->height);


        uint32_t *word;
        for( int i = 0 ; i < comp_image->height; i ++){
                for(int j = 0; j < comp_image->width; j++){
                        word = UArray2_at(comp_image, j, i);
                        for (unsigned k = 0; k < 4; k++) {
                                putchar(Bitpack_getu(*(uint64_t *)word,
                                                         W_SIZE, k * W_SIZE));
                        }
                }
        }

}
