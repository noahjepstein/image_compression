An image compressor/decompressor written in C for Machine Structure at Tufts University.

Includes a number of performance features including bitpacking and blocking to reduce cache miss rate.

Usage: To compress:
``` $ ./40image -c [infile.ppm] > [outfile.bin]```

To decompress:
```$ ./40image -d [infile.bin] > [outfile.ppm]```

To test:
```$ ./40image -t [infile.ppm] > [outfile.ppm]```

============================== 40image =======================================

1. What problem are you trying to solve?

We are trying to compress and decompress full-color portable pixmap images and
compress binary image files.

2. What example inputs will help illuminate the problem?

If we compress then decompress ppm and binary images using our program, we can
simply check for differences between the input and output files. After
compression and decompression, the difference between the input and output
should be minimal if not non-existent.

3. What example outputs go with those emxample inputs?

For each input, we will compress the image into a binary image file, then
decompress it into a pixmap - so the output should be roughly identical to the
input.

4. Into what steps or subproblems can you break down the problem?

    a) Processing command line arguments
    b) Reading in images
        i) Check (and possibly trim) the last rows or columns of images files
           so the image has an even number of rows or columns.
    c) Change the pixmap representation to a floating-point representation:
       each pixel changes from a struct of RGB values to component-video color
       space.
    d) Pack each 2x2 block into a 32-bit word:
        i)  Find the chroma elements of each pixel by taking the average
            values of the four pixels in the 2x2 block.
        ii) Convert the chroma elements into four-bit values using a function
            provided by Comp40 staff.

        iii) Transform the luminance values of the 2x2 block into cosine
        coefficients a, b, c, and d, where a represents average brightness of
        the block, b, represents the degree to which the image gets brighter
        from top to bottom, c represents the degree to which the image gets
        brighter from left to right, and d represents the degree to which
        pixels on one diagonal are brighter than those on the other diagonal.

        iv) Check that b, c, and d are within an appropriate range (-.3 to .3)

        v)  Convert b, c, and d to 5-bit sine values.

    e) Pack luminance elements and the chroma elements into 32-bit words.

    f) Write the compressed image to stdout.

    ** To write a decompressed image from the compressed binary image, we will
       perform the reverse operation of the above steps. **

5. What data are in each subproblem?

    - We'll have to deal with color pixmaps made of RGB structs
    - We'll have to deal with 2x2 blocks of RGB pixels
    - Chroma elements of a 2x2 block
    - Luminance elements of a 2x2 block
    - 32 bit words that represent all the data about a 2x2 block
    - Binary image file that represents the compressed image

6. What code or algorithms go with that data?

    We'll need algorithms to:

    - store and read in pixmaps (Using pnm interface)
    - change pixels from RGB pixels to video-colorspace
    - convert 2x2 blocks to chroma elements and luminance values
    - Transform luminance values to cosine coefficients
    - Convert the coefficients to five-bit sine values
    - Pack all elements into a 32-bit word
    - Write a compressed binary image to stdout

7. What abstractions will you use to help solve the problem?

    We'll use the pnm interface to read and write pnm files, which will use
    "blocked" methods - this way, the image will be stored as an array of
    2x2 pixel blocks.

    We'll use a Uarray2 to store our compressed image as an array of 32-bit
    words.

8. What invariant properties should hold during the solution of the problem?

    - A 2x2 block of pixels in the original pixmap wil be represented by one
      32-bit word in our compresssed image.
    - The width and height of the compressed image will correspond with the
      same properties of the original image by the equations:
        w_compressed = w_orig / 2
        h_compressed = h_orig / 2
    - Each 2x2 block is also represented by its chroma and luminance values.

9. What algorithms might help solve the problem?

    Most of the mathematical algoritms are specified for us by Comp40 staff,
    but we will use a few of our own algorithms:
        - Storing the chroma and luminance values of a 2x2 block as a 32-bit
          word
        - Storing all the 32-bit words in the appropriate places in a Uarray2
        - Printing an image in compressed format to stdout
        ** We will also use the inverse of the above algorithms, both included
           in the assignment and implemented by the authors. **

10. What are the major components of your program, and what are their
    interfaces?

    - Each subproblem that we mentioned in 4 constitutes a major component of
      our program. Reading in Pnm images will use the pnm interface, storing
      the array of pixels will use the Uarray2b interface, and storing words
      in the binary image file will use the Uarray2 interface.

11. How do the components of your progrma interact?

    Our image will be compressed using a tiered structure: each step
    represents a level of the conversion process, and each part acts
    sequentially on our data to change each element of the image from its
    original to compressed form (or from compressed form to original form).

12. What test cases will you use to convince yourself that your program works?

    We will use regular pnm images, compressing and decompressing them and
    then comparing input and output with ppmdiff to ensure that the data in
    the original image carries through to the compressed one and back again.

13. What arguments will you use to convince a skeptical audience that your
    program works?

    Testing with ppmdiff will ensure a consistent way to measure differences
    between input and output images.

    If each 2x2 block of RGB pixels has chroma and luminance values that fully
    specify the way brightness and color intensity vary within a block, and
    the chroma and luminance values are smaller to store than the 2x2 block of
    RGB pixels, a stored array of 32 bit words that contain chroma and
    luminance values will be smaller than the original image (thus
    compressed). The inverse is true as well.

============================= bitpack ========================================

1. What is the abstract datatype you're trying to represent?

We're trying to represent a 64-bit word such that you can access and modify
its components.

2 and 3: What functions will you offer and what contracts will they meet? What
examples do you have of what the functions are supposed to do?

We will offer the following:

    - bool Bitpack_fitsu(uint64_t n, unsigned int width);
        --Determines whether an unsigned int n can be represented using width
          bits.

          Example usage: Bitpack_fitsu(8, 4) == true;

    - bool Bitpack_fitss(int64_t n, unsigned int width);
        -- Determines whether a signed int n can be represented with width
           bits.

           Example usage: Bitpack_fitsu(8, 4) == false;

    - uint64_t Bitpack_getu(uint64_t word, unsigned int width, unsigned int
                                                                          lsb)
        -- Bitpack_getu extracts the value contained in width bits starting at
           the least significant bit in the value, lsb. The value extracted
           will be an unsigned int. If width == 0, Bitpack_getu returns zero.
           CRE if width is less than zero or greater than 64, or if lsb +
           width > 64.

           Example usage: Bitpack_getu(0x200, 3, 16) returns the three bits in
           ascending order which start at bit 16 - in this case, as an
           unsigned integer.

    - int64_t Bitpack_gets(uint64_t word, unsigned int width, unsigned int
                                                                          lsb)
        -- Bitpack_gets extracts the value contained in width bits starting at
           the least significant bit in the value, lsb. The value extracted
           will be a signed integer, but the function still takes an unsigned
           word, per programming convention. If width == 0, Bitpack_getu
           returns zero. CRE if width is less than zero or greater than 64, or
           if lsb + width > 64.

           Example usage: Bitpack_gets(0x200, 3, 16) returns the three bits in
           ascending order which start at bit 16 - in this case, as a
           signed integer.

    - uint64_t Bitpack_newu(uint64_t word, unsigned int width, unsigned int
                                                          lsb, uint64_t value)
            -- return a word identical to the original word except modified to
               hold value of width length, starting at lsb. CRE to call
               bitpack new on a bit that falls out of 0 <= w <= 64. CRE if
               (w + lsb) > 64. Raises exception Bitpack_Overflow if value does
               not fit within width unsigned bits.

            Example usage: Bitpack_newu(0x200, 6, 0, 55) returns a new word at
            a different address than 0x200, identical to the original word
            except the six bit unsigned integer starting at 0 is changed to
            represent 55.

    - uint64_t Bitpack_news(uint64_t word, unsigned int width, unsigned int
                                                           lsb, int64_t value)

            -- return a word identical to the original word except modified to
               hold value of width length, starting at lsb. CRE to call
               bitpack new on a bit that falls out of 0 <= w <= 64. CRE if
               (w + lsb) > 64. Raises exception Bitpack_Overflow if value does
               not fit within width signed bits.

            Example usage: Bitpack_news(0x200, 6, 0, 30) returns a new word at
            a different address than 0x200, identical to the original word
            except the six bit signed integer starting at 0 is changed to
            represent 30.


    The signed and unsigned functions will be impelemented independently of
    one another; the existence of one set of functions will in no way affect
    that of the other.

4. What representation will you use and what invariants will it satisfy?

We're representing a 64-bit word as an unsigned int in memory.

It will satisfy the following invariants:

    -- A word is always an unsigned 64 bit integer.
    -- Width always meets the condition 0 <= w <= 64.
    -- Width + lsb must always meet the condition w + lsb <= 64.
    -- We will always read in little-endian order, starting at lsb and going
       left.
    -- lsb must always be less than or equal to 64.

5. When a representation satisfies all invariants, what abstract thing from
   step one does it represent?

It represents a 64-bit word whose elements can be accessed and modified
individually or as a segment of width bits. Its elements can also be
interpreted and modified and returned as unsigned or signed integers.

6. What test cases have you devised?

    It's a CRE to call any functions using width under zero or over 64.
    It's a CRE to call a function using width + lsb > 64.
    It's a CRE to call a function using NULL as a word.
    It's a violation of the contracts of all Bitpack methods to use an out of
    bounds address for a word.

    We will test with widths of zero to ensure that any Bitpack methods using
    width of zero return zero as a a value for get functions, or to ensure
    that "new" functions return a word identical to the original when width is
    zero, regardless of value or lsb. We will test all cases that give a CRE
    or violate contract to ensure consistency in failure. We will test with
    various 64-bit words with known elements to check that they can be
    retrieved, interpreted, and written accurately. We will test both signed
    and unsigned fit functions with values that fit and do not fit to ensure
    correctness.

7. What programming idioms will you need?

    - The idiom for shifting left and right for varying magnitudes of shift
    - The idiom for reading within a word written in little-endian order
    - The idiom for isolating a width-size segment that starts at a least
      significant bit.
    - The idiom for using a mask to replace a segment within a word

==============================================================================

How will your design enable you to do well on the challenge problem in Section
2.3 on page 13?

    We'll use a modular design with methods whose implementions rely on single
    points of truth. Our design will feature methods that take care of our
    programming idioms seperately so that we compartmentalize portions of our
    code that deal with the specific format of a word. We will use modules
    that encapulate "secrets" in our design: we'll use this as a layer of
    abstraction, so other modules will not have to be adapted to deal with the
    "secret" parts of our design.

    For example: if the challenge problem involved switching from little-
    endian bit-order to big-endian, we would have one module that deals with
    specific locations of bits within a word - and only that module would have
    to change to change the order in which we read bits.

==============================================================================

An image is compressed and then decompressed. Identify all the places where
information could be lost. Then it's compressed and decompressed again. Could
more information be lost? How?

    Compressing and decompressing an image the first time can introduce the
    possibility of data loss. However, the second time an image is compressed,
    it will not lose any more data than it lost in the first cycle. When
    information stored in a block of pixels is reduced to floating point
    values, the block of four pixels is instead represented as an average of
    the four values. There are multiple combinations of pixels that would
    simplify to the same average values, so upon decompression, the values of
    the pixels would be normalized with respect to each other. If the image is
    recompressed, the pixels would simplify to the same average values, so it
    would be impossible to lose any more information. You can't make the
    pixels more normalized with respect to each other.

==============================================================================
