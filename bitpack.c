/* Filename:         bitpack.c
 * Last Modified:    Mar 1st, 2015
 *
 * Acknowledgements: See README.txt
 *
 * Description:      Bitpack is a module with functions for bitpacking and
 *                   manipulating bit fields of signed and unsigned int values
 *                   within the uint64_t type. It features functions for
 *                   checking whether ints and unsigneds fit within numbers of
 *                   bits, for pulling values of bitfields, and for inserting
 *                   values into bitfields.
 */



#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "except.h"
#include <bitpack.h>
#include <stdio.h>

const uint64_t ALL_ONES = 0xFFFFFFFFFFFFFFFF;
const uint64_t ALL_ZEROS = 0x0000000000000000;

/* shifts val left by n bits */
static inline uint64_t ushift_l(uint64_t val, unsigned n);

/* shifts val right by n bits */
static inline uint64_t ushift_r(uint64_t val, unsigned n);


/* Description: Checks if an unsigned value n fits within a bitfield of size
 *              width.
 *
 * Input:       n, a value to check; width, the size of a bitfield.
 * Output:      True if it fits.
 */
bool Bitpack_fitsu(uint64_t n, unsigned width)
{
        assert(width <= 64);
        uint64_t maxval = ushift_l((uint64_t)1, width);
        return maxval >= n;
}


/* Description: Checks if a signed value n fits within a bitfield of size
 *              width.
 *
 * Input:       n, a value to check; width, the size of a bitfield.
 * Output:      True if it fits.
 */
bool Bitpack_fitss( int64_t n, unsigned width)
{
        uint64_t un = (uint64_t) n;
        assert(width <= 64);
        uint64_t maxval = ushift_l((uint64_t)1, width);
        return un <= maxval;
}


/* Description: Retrieves an unsigned value from a bitfield starting at
 *              least-significant-bit lsb, of size width, contained in word.
 *
 * Input:       word, the container; width, the size of a bitfield. lsb, the
 *              location of the bitfield.
 * Output:      True if it fits.
 */
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        assert(width + lsb < 64);
        uint64_t mask = ALL_ZEROS;

        if (width == 0) {
            return 0;
        }

        for (unsigned i = lsb; i < width + lsb; i++) {
            mask +=  ushift_l((uint64_t)1, i);
        }

        uint64_t shift = (uint64_t) (mask & word);
        shift = ushift_r(shift, lsb);

        return shift;
}


/* Description: Puts an unsigned value into a bitfield in word starting at
 *              least-significant-bit lsb, of size width.
 *
 * Input:       word, the container; width, the size of a bitfield. lsb, the
 *              location of the bitfield, value, the value to store.
 * Output:      The new word that contains the stored value.
 */
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb,
                                                               uint64_t value)
{
        assert(Bitpack_fitsu(value, width));
        assert(width <= 64);
        assert(lsb <= 64);
        assert(lsb + width < 64);


        uint64_t mask = ALL_ZEROS;

        for (unsigned i = lsb; i < width + lsb; i++) {
            mask += ushift_l((uint64_t)1, i);
        }

        mask = (uint64_t) ~mask;

        uint64_t newword = (uint64_t) (mask & word);
        uint64_t s_val = (uint64_t) ushift_l(value, lsb);
        s_val = (uint64_t) s_val | newword;
        return (uint64_t)(s_val);
}


/* Description: Retrieves a signed value from a bitfield starting at
 *              least-significant-bit lsb, of size width, contained in word.
 *
 * Input:       word, the container; width, the size of a bitfield. lsb, the
 *              location of the bitfield.
 * Output:      True if it fits.
 */
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        assert(width + lsb < 64);
        uint64_t mask = ALL_ZEROS;

        if (width == 0) {
            return 0;
        }

        for (unsigned i = lsb; i < width + lsb; i++) {
            mask +=  ushift_l((uint64_t)1, i);
        }

        uint64_t shift = (uint64_t) (mask & word);
        shift = (int64_t)ushift_r(shift, lsb);

        return shift;
}


/* Description: Puts a signed value into a bitfield in word starting at
 *              least-significant-bit lsb, of size width.
 *
 * Input:       word, the container; width, the size of a bitfield. lsb, the
 *              location of the bitfield, value, the value to store.
 * Output:      The new word that contains the stored value.
 */
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,
                                                                int64_t value)
{

        assert(Bitpack_fitss(value, width));
        assert(width <= 64);
        assert(lsb <= 64);
        assert(lsb + width < 64);

        uint64_t uval = (uint64_t) value;

        uint64_t mask = ALL_ZEROS;

        for (unsigned i = lsb; i < width + lsb; i++) {
            mask += ushift_l((uint64_t)1, i);
        }

        mask = (uint64_t) ~mask;

        uint64_t newword = (uint64_t) (mask & word);
        uint64_t s_val = (uint64_t) ushift_l(uval, lsb);
        s_val = (uint64_t) s_val | newword;
        return (uint64_t)(s_val);
}



/* shifts val left by n bits, and returns 0 if shifted by 64. */
static inline uint64_t ushift_l(uint64_t val, unsigned n)
{
        if (n >= 64) {
            return (uint64_t)0;
        }
        uint64_t newval = val;
        newval = (uint64_t)(newval << n);
        return (uint64_t)newval;
}

/* shifts val right by n bits , and returns 0 if shifted by 64. */
static inline uint64_t ushift_r(uint64_t val, unsigned n)
{
        if (n >= 64) {
            return (uint64_t)0;
        }
        uint64_t newval = val;
        newval = (uint64_t)(newval >> n);
        return (uint64_t)newval;
}
