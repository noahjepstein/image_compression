#line 49 "www/solutions/uarray2b.nw"
#include <math.h>
#include "assert.h"
#include "mem.h"
#include "uarray.h"
#include "uarray2.h"
#include "uarray2b.h"

#define T UArray2b_T

struct T { // represents a 2D array of cells each of size 'size'
  int width, height;
  unsigned blocksize;
  unsigned size;
  UArray2_T blocks;
    /* matrix of blocks, each blocksize * blocksize */
    /* matrix dimensions are width and height divided by blocksize,
       rounded up */
    /* a block is an Array_T of length blocksize * blocksize and size 'size' */
    /* invariant relating cells in blocks to cells in the abstraction
       described in section on coordinate transformations below */
};
#line 78 "www/solutions/uarray2b.nw"
#include <stdio.h>  /* include so we can print diagnostics */

T UArray2b_new(int width, int height, int size, int blocksize) {
  assert(blocksize > 0);
  T array;
  NEW(array);
  array->width  = width;
  array->height = height;
  array->size   = size;
  array->blocksize = blocksize;
  array->blocks = UArray2_new((width  + blocksize - 1) / blocksize,
                             (height + blocksize - 1) / blocksize,
                             sizeof(UArray_T));
  int xblocks = UArray2_width (array->blocks);
  int yblocks = UArray2_height(array->blocks);
  for (int i = 0; i < xblocks; i++) {
    for (int j = 0; j < yblocks; j++) {
      UArray_T *block = UArray2_at(array->blocks, i, j);
      *block = UArray_new(blocksize * blocksize, size);

#line 148 "www/solutions/uarray2b.nw"
if (0) {
  fprintf(stderr, "Allocated %p; put %p at %p\n",
          (void *)*block, (void *)*(UArray_T*)UArray2_at(array->blocks, i, j),
          UArray2_at(array->blocks, i, j));
}
#line 98 "www/solutions/uarray2b.nw"
    }
  }
  return array;
}
#line 107 "www/solutions/uarray2b.nw"
void UArray2b_free(T *array2b) {
  int i;
  assert(array2b && *array2b);
  T array = *array2b;
  int xblocks = UArray2_width (array->blocks);
  int yblocks = UArray2_height(array->blocks);
  assert(UArray2_size(array->blocks) == sizeof(UArray_T));
  for (i = 0; i < xblocks; i++) {
    for (int j = 0; j < yblocks; j++) {
      UArray_T *p = UArray2_at(array->blocks, i, j);
      UArray_free(p);
    }
  }
  UArray2_free(&(*array2b)->blocks);
  FREE(*array2b);
}
#line 130 "www/solutions/uarray2b.nw"
T UArray2b_new_64K_block(int width, int height, int size) {
  int blocksize = (int) floor(sqrt((double) (64 * 1024) / (double) size));
  if (blocksize == 0)
    blocksize = 1;
  assert((blocksize+1) * (blocksize+1) * size > 64 * 1024); // as big as possible
  if (size <= 64 * 1024)
    assert(blocksize * blocksize * size <= 64 * 1024); // no bigger
  return UArray2b_new(width, height, size, blocksize);
}
#line 179 "www/solutions/uarray2b.nw"
void *UArray2b_at(T array2b, int i, int j) {
  assert(i >= 0 && j >= 0);
  assert(i < array2b->width && j < array2b->height); // avoid unused cells
  int b  = array2b->blocksize;
  int bx = i / b;  // block x coordinate
  int by = j / b;  // block y coordinate
  UArray_T *blockp = UArray2_at(array2b->blocks, bx, by);
  return UArray_at(*blockp, (i % b) * b + j % b);
}
#line 199 "www/solutions/uarray2b.nw"
void UArray2b_map(T array2b,
    void apply(int i, int j, T array2b, void *elem, void *cl), void *cl) {
  assert(array2b);
  int h = array2b->height;
  int w = array2b->width;
  int b = array2b->blocksize;
  UArray2_T blocks = array2b->blocks;
  int bw = UArray2_width(blocks);
  int bh = UArray2_height(blocks);

  for (int bx = 0; bx < bw; bx++) {
    for (int by = 0; by < bh; by++) {
      UArray_T *blockp = UArray2_at(blocks, bx, by);
      UArray_T block = *blockp;
      int len = UArray_length(block);
      int i0 = b * bx; // (i0,j0) correspond to upper left
      int j0 = b * by; // corner of block (bx, by)
      for (int cell = 0; cell < len; cell++) {
        int i = i0 + cell / b;
        int j = j0 + cell % b;
        if (i < w && j < h) // measured overhead 0.5% to 1.5%
          apply(i, j, array2b, UArray_at(block, cell), cl);
      }
    }
  }
}
#line 239 "www/solutions/uarray2b.nw"
int UArray2b_height(T array2b) {
  assert(array2b);
  return array2b->height;
}
int UArray2b_width(T array2b) {
  assert(array2b);
  return array2b->width;
}
int UArray2b_size(T array2b) {
  assert(array2b);
  return array2b->size;
}
int UArray2b_blocksize(T array2b) {
  assert(array2b);
  return array2b->blocksize;
}
#line 260 "www/solutions/uarray2b.nw"
int UArray2b_version_uses_UArray2_T = 1;
