/* Filename:         packpix.c
 * Last Modified:    Mar 1st, 2015
 *
 * Acknowledgements: See README.txt
 *
 * Description:      PACKPIX is a module that implements functions which turn
 *                   UArray2bs of component-video pixels into blockwise
 *                   averages for 2x2 blocks of component video pixels,
 *                   quantizes the averages to indices, then packs the
 *                   quantized indicies into 32-bit words which are stored in
 *                   a Hanson UArray2_T. Reverse to decompress.
 */


#include "packpix.h"
#include "bitpack.h"
#include "uarray2.h"
#include "uarray.h"
#include "pnm.h"
#include "types.h"
#include <stdlib.h>
#include <assert.h>
#include <arith40.h>
#include <math.h>


const int WORDSIZE  = 8;
const int BLOCK_LEN = 4;
const int BLK_SIZE  = 2;

const float CV_HIGH_BOUND = 0.3;
const float  CV_LOW_BOUND = -0.3;
const float AV_HIGH_BOUND = 0.5;
const float  AV_LOW_BOUND = -0.5;

const float LUM_HI = 1.0;
const float LUM_LO = 0.0;
const float PR_HI  = 0.5;
const float PR_LO  = -0.5;
const float PB_HI  = 0.5;
const float PB_LO  = -0.5;

const unsigned  QCVA_HIGH = 512;
const unsigned  QCVA_LOW  =   0;
const signed    QCVB_HIGH = 15;
const signed    QCVB_LOW  = -15;
const signed    QCVC_HIGH = 15;
const signed    QCVC_LOW  = -15;
const signed    QCVD_HIGH = 15;
const signed    QCVD_LOW  = -15;
const unsigned  QCVQPR_HIGH = 8;
const unsigned  QCVQPR_LOW  = 0;
const unsigned  QCVQPB_HIGH = 8;
const unsigned  QCVQPB_LOW  = 0;

const int   QUANT_FACTOR =   64;
const int A_QUANT_FACTOR =  511;

const int LSB_A = 26;
const int LSB_B = 20;
const int LSB_C = 14;
const int LSB_D  = 8;
const int LSB_PB = 4;
const int LSB_PR = 0;
const int ABCD_WIDTH = 6;
const int PRPB_WIDTH = 4;


static inline void clip_float_cv(struct float_comp_vid *fcv);
static inline void clip_quant(struct quant_comp_vid *qcv);
static inline void clip_cv(struct comp_vid *cv);

void apply_block_to_float(int i, int j, UArray2b_T cv_array, void *elem,
                                                           void *arr_closure);

void apply_float_to_quant(int i, int j, UArray2_T fcv_array, void *elem,
                                                                    void *cl);

void apply_quant_pack(int i, int j, UArray2_T qcv_array, void *elem,
                                                                    void *cl);

void apply_quant_unpack(int i, int j, UArray2_T pack_array, void *elem,
                                                                    void *cl);

void apply_quant_to_float(int i, int j, UArray2_T qcv_array, void *elem,
                                                                    void *cl);

void apply_float_to_block(int i, int j, UArray2_T fcv_array,
                                                  void *elem, void *arr2b_cl);



/*==========================================================================*/


/* Description: Converts a pixelwise component video array into an array of
 *              32-bit words, using a series of apply functions.
 *
 * Input:       UArray2b of component video pixels.
 * Output:      UArray2 of 32 bit words, each representing a 2x2 pixel block.
 */
UArray2_T comp_vid_to_word(UArray2b_T cv_array)
{
        /*make a target array of blockwise component video structs for map */
        UArray2_T avg_float_arr =
                            UArray2_new(cv_array->width / cv_array->blocksize,
                                       cv_array->height / cv_array->blocksize,
                                               sizeof(struct float_comp_vid));
        UArray2_map_row_major(cv_array->blocks,
                      &apply_block_to_float, avg_float_arr);


        /*make a target array of quant_component video structs for map */
        UArray2_T quant_arr =
                            UArray2_new(cv_array->width / cv_array->blocksize,
                                       cv_array->height / cv_array->blocksize,
                                               sizeof(struct quant_comp_vid));
        UArray2_map_row_major(avg_float_arr, &apply_float_to_quant,quant_arr);

        UArray2_free(&avg_float_arr);

        /*make a target array of 32 bit words where we'll store final data */
        UArray2_T word_arr =UArray2_new(cv_array->width / cv_array->blocksize,
                                       cv_array->height / cv_array->blocksize,
                                                           sizeof(unsigned*));

        UArray2_map_row_major(quant_arr, &apply_quant_pack, word_arr);

        UArray2_free(&quant_arr);

        return word_arr;
}

/* Description: Converts a UArray2 of 32 bit words into a Uarray2b of compnent
 *              video pixels.
 *
 * Input:       UArray2 of 32 bit words, each representing a 2x2 pixel block.
 * Output:      UArray2b of component video pixels.
 */
UArray2b_T word_to_comp_vid(UArray2_T word_arr)
{
        /* make a uarray2 to hold all the quantized quant_comp_vid structs
         * after we unpack them
         */
        UArray2_T quant_arr =
                           UArray2_new(word_arr->width, word_arr->height,
                                               sizeof(struct quant_comp_vid));
        UArray2_map_row_major(word_arr, &apply_quant_unpack, quant_arr);


        /*turn the quant_comp_vid array into a float_comp_vid array */
        UArray2_T float_arr = UArray2_new(word_arr->width, word_arr->height,
                                          sizeof(struct float_comp_vid));
        UArray2_map_row_major(quant_arr, &apply_quant_to_float, float_arr);


        UArray2_free(&quant_arr);


        /*turn the float comp vid array into a pixelwise comp_vid array */
        UArray2b_T cv_array = UArray2b_new(float_arr->width * 2,
                                           float_arr->height * 2,
                                           sizeof(struct comp_vid), BLK_SIZE);
        UArray2_map_row_major(float_arr, &apply_float_to_block,
                                                           cv_array->blocks);

        UArray2_free(&float_arr);

        return cv_array;
}






/* Description: Apply function that maps through a Uarray2 of UArrays, each
 *              Uarray reprsenting a 2x2 block of CV pixels. Turns all the
 *              blocks into float_comp_vids, which hold average values about
 *              the entire block for simplification.
 *
 * Input:       Takes i and j indices of the block, pointer to the block
 *              itself, new target array passed as closure where we'll place
 *              float_comp_vid structs.
 * Output:      UArray2 of block average structs as closure.
 */
void apply_block_to_float(int i, int j, UArray2b_T cv_array, void *felem,
                                                            void *arr_closure)
{
        (void)cv_array;

        UArray2_T float_array = arr_closure;
        UArray_T block = *(UArray_T *)felem;
        struct comp_vid *cvpixel;
        struct float_comp_vid *fcv = UArray2_at(float_array, i, j);
        float total_pr = 0;
        float total_pb = 0;

        assert(block != NULL);

        struct comp_vid *cv1 = UArray_at(block, 0);
        struct comp_vid *cv2 = UArray_at(block, 1);
        struct comp_vid *cv3 = UArray_at(block, 2);
        struct comp_vid *cv4 = UArray_at(block, 3);

        assert(cv1 != NULL && cv2 != NULL && cv3 != NULL && cv4 != NULL);

        float y1 = cv1->lum;
        float y2 = cv2->lum;
        float y3 = cv3->lum;
        float y4 = cv4->lum;

        for (int k = 0; k < BLOCK_LEN; k++) {
            cvpixel = UArray_at(block, k);
            total_pb  += cvpixel->pb;
            total_pr  += cvpixel->pr;
        }

        //some calculations to average the values of the pixels.
        //avg brightness, left to right brightness, top to bottom, diagonal.
        fcv->a      = (y1 + y2 + y3 + y4) / BLOCK_LEN;
        fcv->b      = (y4 + y3 - y2 - y1) / BLOCK_LEN;
        fcv->c      = (y4 - y3 + y2 - y1) / BLOCK_LEN;
        fcv->d      = (y4 - y3 - y2 + y1) / BLOCK_LEN;

        //average color values
        fcv->pb_avg = total_pb / BLOCK_LEN;
        fcv->pr_avg = total_pr / BLOCK_LEN;

        clip_float_cv(fcv);

        struct float_comp_vid *elem = UArray2_at(float_array, i, j);
        elem->a = fcv->a;
        elem->b = fcv->b;
        elem->c = fcv->c;
        elem->d = fcv->d;
        elem->pb_avg = fcv->pb_avg;
        elem->pr_avg = fcv->pr_avg;
}




/* Description: Apply function that maps through a Uarray2 of float comp vids,
 *              each of which represents a 2x2 block of pixels. Turns all the
 *              averages to 2x2 blocks of CV pixels.
 *
 * Input:       Takes i and j indices of the block, pointer to the block
 *              itself, new target array passed as closure where we'll place
 *              float_comp_vid structs.
 * Output:      UArray2b of CV pixels as closure.
 */
void apply_float_to_block(int i, int j, UArray2_T float_array, void *elem,
                                                               void *arr2b_cl)
{
        struct float_comp_vid *fcv = UArray2_at(float_array, i, j);
        UArray2_T blocks = arr2b_cl;
        assert(blocks != NULL);
        assert(fcv != NULL);
        (void)elem;

        clip_float_cv(fcv);

        float a = fcv->a;
        float b = fcv->b;
        float c = fcv->c;
        float d = fcv->d;

        /*calculating the cosine transformed luminance values */
        float lum1 = (a - b - c + d);
        float lum2 = (a - b + c - d);
        float lum3 = (a + b - c - d);
        float lum4 = (a + b + c + d);

        UArray_T block = *(UArray_T*)UArray2_at(blocks, i, j);

        //get each pixel from the UArray
        struct comp_vid *pix1 = UArray_at(block, 0);
        struct comp_vid *pix2 = UArray_at(block, 1);
        struct comp_vid *pix3 = UArray_at(block, 2);
        struct comp_vid *pix4 = UArray_at(block, 3);

        pix1->lum = lum1;
        pix1->pb = fcv->pb_avg;
        pix1->pr = fcv->pr_avg;
        clip_cv(pix1);

        pix2->lum = lum2;
        pix2->pb = fcv->pb_avg;
        pix2->pr = fcv->pr_avg;
        clip_cv(pix2);

        pix3->lum = lum3;
        pix3->pb = fcv->pb_avg;
        pix3->pr = fcv->pr_avg;
        clip_cv(pix3);

        pix4->lum = lum4;
        pix4->pb = fcv->pb_avg;
        pix4->pr = fcv->pr_avg;
        clip_cv(pix4);
}


/* Description: Apply function that maps through a Uarray2 of float comp vids,
 *              each of which represents a 2x2 block of pixels. Quantizes all
 *              values using a struct that holds bit fields. Uses arith
 *              functions to perform nonlinear quantization.
 *
 * Input:       Takes i and j indices of the fcv, pointer to the fcv itself,
 *              new target array passed as closure where we'll place
 *              quantized_comp_vid structs.
 * Output:      UArray2 of QCVs as closure.
 */
void apply_float_to_quant(int i, int j, UArray2_T float_array, void *elem,
                                                                     void *cl)
{
        //converts float values to quantized values and stores them in
        //quantized structure.

        struct float_comp_vid *fcv = elem;
        UArray2_T quant_arr = cl;
        struct quant_comp_vid *qcv = UArray2_at(quant_arr, i, j);

        unsigned b = Arith40_index_of_chroma(fcv->pb_avg);
        unsigned r = Arith40_index_of_chroma(fcv->pr_avg);
        qcv->qpb = b / 2;
        qcv->qpr = r / 2;
        qcv->a   = (fcv->a * QUANT_FACTOR);
        qcv->b   = (fcv->b * QUANT_FACTOR);
        qcv->c   = (fcv->c * QUANT_FACTOR);
        qcv->d   = (fcv->d * QUANT_FACTOR);

        clip_quant(qcv);
        (void)float_array;
}


/* Description: Apply function that maps through a Uarray2 of quantized
 *              component video values and packs each quanitzed element into
 *              a 32 bit word as a client of the bitpack module.
 *
 * Input:       Takes i and j indices of the QCV, pointer to the QCV itself,
 *              new target array passed as closure where we'll place
 *              words.
 * Output:      UArray2 of 32 bit words as closure.
 */
void apply_quant_pack(int i, int j, UArray2_T quant_arr, void *elem,
                                                                     void *cl)
{
        struct quant_comp_vid *qcv = elem;

        UArray2_T word_array = cl;
        uint32_t *word = UArray2_at(word_array, i, j);

        //each element of the quantized struct has a location in the 32b word
        *word = (uint32_t) Bitpack_newu(* (uint64_t *)word, ABCD_WIDTH, LSB_A,
                                                            (uint64_t)qcv->a);
        *word = (uint32_t) Bitpack_news(* (uint64_t *)word, ABCD_WIDTH, LSB_B,
                                Bitpack_gets((int64_t)qcv->b, ABCD_WIDTH, 0));
        *word = (uint32_t) Bitpack_news(* (uint64_t *)word, ABCD_WIDTH, LSB_C,
                                Bitpack_gets((int64_t)qcv->c, ABCD_WIDTH, 0));
        *word = (uint32_t) Bitpack_news(* (uint64_t *)word, ABCD_WIDTH, LSB_D,
                                Bitpack_gets((int64_t)qcv->d, ABCD_WIDTH, 0));
        *word = (uint32_t) Bitpack_newu(*(uint64_t *)word, PRPB_WIDTH, LSB_PB,
                                                          (uint64_t)qcv->qpb);
        *word = (uint32_t) Bitpack_newu(*(uint64_t *)word, PRPB_WIDTH, LSB_PR,
                                                          (uint64_t)qcv->qpr);

        (void)quant_arr;
}


/* Description: Apply function that maps through a Uarray2 of 32 bit words
 *              and pulls out the quantized component video values.
 *
 * Input:       Takes i and j indices of the word, pointer to the word itself,
 *              new target array passed as closure where we'll place
 *              QCVs.
 * Output:      UArray2 of QCVs as closure.
 */
void apply_quant_unpack(int i, int j, UArray2_T word_array, void *elem,
                                                                     void *cl)
{
        uint64_t *word = UArray2_at(word_array, i, j);
        UArray2_T quant_arr = cl;
        struct quant_comp_vid *qcv = UArray2_at(quant_arr, i , j);

        //each element of the quantized struct has a location in the 32b word
        qcv->a =  (uint32_t) Bitpack_getu(*(uint64_t*)word, ABCD_WIDTH,
                                                                    LSB_A);
        qcv->b = (int32_t) Bitpack_gets(*(uint64_t *)word, ABCD_WIDTH, LSB_B);
        qcv->c = (int32_t) Bitpack_gets(*(uint64_t *)word, ABCD_WIDTH, LSB_C);
        qcv->d = (int32_t) Bitpack_gets(*(uint64_t *)word, ABCD_WIDTH, LSB_D);
        qcv->qpb = (uint32_t) Bitpack_getu(*(uint64_t *)word, PRPB_WIDTH,
                                                                    LSB_PB);
        qcv->qpr = (uint32_t) Bitpack_getu(*(uint64_t *)word, PRPB_WIDTH,
                                                                    LSB_PR);

        (void)elem;
}


/* Description: Apply function that maps through a Uarray2 of quantized comp
 *              video values and unquantizes them, turning them back into
 *              floats.
 *
 * Input:       Takes i and j indices of the QCV, pointer to the QCV itself,
 *              new target array passed as closure where we'll place
 *              FCV.
 * Output:      UArray2 of FCVs as closure.
 */
void apply_quant_to_float(int i, int j, UArray2_T quant_arr, void *elem,
                                                                     void *cl)
{
        //converts quant_comp_vid structs to float_comp_vid structs and stores
        // them in the word_array
        struct quant_comp_vid *qcv = elem;
        UArray2_T float_arr = cl;

        assert(qcv != NULL);
        assert(float_arr != NULL);
        struct float_comp_vid *fcv = UArray2_at(float_arr, i, j);
        assert(fcv != NULL);

        //deindex all quantized values
        fcv->pb_avg = (float) Arith40_chroma_of_index(qcv->qpb * 2) ;
        fcv->pr_avg = (float) Arith40_chroma_of_index(qcv->qpr * 2) ;
        fcv->a   = (float)qcv->a / QUANT_FACTOR;
        fcv->b   = (float)qcv->b / QUANT_FACTOR;
        fcv->c   = (float)qcv->c / QUANT_FACTOR;
        fcv->d   = (float)qcv->d / QUANT_FACTOR;

        (void)quant_arr;
}

/* ============================== CLIP FUNCTIONS ======================== */

/*function for clipping component values in a pixel to appropriate high and
 *low values
 */
static inline void clip_cv(struct comp_vid *cv)
{
    if (cv->lum > LUM_HI) {
        cv->lum = LUM_HI;
    } else if (cv->lum < LUM_LO) {
        cv->lum = LUM_LO;
    }

    if (cv->pb > PB_HI) {
        cv->pb = PB_HI;
    } else if (cv->pb < PB_LO) {
        cv->pb = PB_LO;
    }

    if (cv->pr > PR_HI) {
        cv->pr = PR_HI;
    } else if (cv->pr < PR_LO) {
        cv->pr = PR_LO;
    }
}


/* clips b, c, and d from float_comp_vid struct so that they remain in bounds
 * for later processing
 */
static inline void clip_float_cv(struct float_comp_vid *fcv)
{

    if (fcv->b > CV_HIGH_BOUND) {
        fcv->b = CV_HIGH_BOUND;

    } else if (fcv->b < CV_LOW_BOUND) {
        fcv->b = CV_LOW_BOUND;
    }

    if (fcv->c > CV_HIGH_BOUND) {
        fcv->c = CV_HIGH_BOUND;

    } else if (fcv->c < CV_LOW_BOUND) {
        fcv->c = CV_LOW_BOUND;
    }

    if (fcv->d > CV_HIGH_BOUND) {
        fcv->d = CV_HIGH_BOUND;

    } else if (fcv->d < CV_LOW_BOUND) {
        fcv->d = CV_LOW_BOUND;
    }

    if(fcv->pr_avg > AV_HIGH_BOUND){
        fcv->pr_avg = AV_HIGH_BOUND;

    } else if (fcv->pr_avg < AV_LOW_BOUND){
        fcv->pr_avg = AV_LOW_BOUND;
    }

    if(fcv->pb_avg > AV_HIGH_BOUND){
        fcv->pb_avg = AV_HIGH_BOUND;

    } else if (fcv->pb_avg < AV_LOW_BOUND){
        fcv->pb_avg = AV_LOW_BOUND;
    }
}


/* clip function for clipping quantized values in a quantized comp vid struct
 * so that they fit inside the appropriate bounds
 */
static inline void clip_quant(struct quant_comp_vid *qcv)
{
        if (qcv->a > QCVA_HIGH) {
                qcv->a = QCVA_HIGH;
        } else if (qcv->a < QCVA_LOW) {
                qcv->a = QCVA_LOW;
        }

        if (qcv->b > QCVB_HIGH) {
                qcv->b = QCVB_HIGH;
        } else if (qcv->b < QCVB_LOW) {
                qcv->b = QCVB_LOW;
        }

        if (qcv->c > QCVC_HIGH) {
                qcv->c = QCVC_HIGH;
        } else if (qcv->c < QCVC_LOW) {
                qcv->c = QCVC_LOW;
        }

        if (qcv->d > QCVD_HIGH) {
                qcv->d = QCVD_HIGH;
        } else if (qcv->d < QCVD_LOW) {
                qcv->d = QCVD_LOW;
        }

        if (qcv->qpb > QCVQPB_HIGH) {
                qcv->qpb = QCVQPB_HIGH;
        } else if (qcv->qpb < QCVQPB_LOW) {
                qcv->qpb = QCVQPB_LOW;
        }

        if (qcv->qpr > QCVQPR_HIGH) {
                qcv->qpr = QCVQPR_HIGH;
        } else if (qcv->qpr < QCVQPR_LOW) {
                qcv->qpr = QCVQPR_LOW;
        }
}
