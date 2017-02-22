README.txt 

Homework 4
Katie Kurtz and Noah Epstein

We recieved some advice and moral support from Noah when we were working near
his office. 40image is also a client of COMP40-solution implementations of 
uarray2b and uarray2. 

As far as we know, all sections of our code are implemented correctly.  
As per our design, if you shift a word by 64, it will return a word with
value 0.

We spent aproximately 40 hours on this project, 3 of which were spent on 
problem analysis, with the remainder spent architecting, coding, and debugging
a solution to the compression problem. Most of the time was spent debugging, 
not writing new code. 

=============================./40image====================================

Design info:

11. How do the components of your program interact? 

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
 
Usage:          To compress:    ./40image -c [infile.ppm] > [outfile.bin]
                To decompress:  ./40image -d [infile.bin] > [outfile.ppm]
                To test:        ./40image -t [infile.ppm] > [outfile.ppm]