/*
    Software DES functions

    SPDX-FileCopyrightText: 1988-1991 Phil Karn <karn@ka9q.net>
    SPDX-FileCopyrightText: 2003 Nikos Mavroyanopoulos <nmav@hellug.gr>

    Taken from libmcrypt (http://mcrypt.hellug.gr/lib/index.html).

    SPDX-License-Identifier: LGPL-2.1-only
*/

/* Software DES functions
 * written 12 Dec 1986 by Phil Karn, KA9Q; large sections adapted from
 * the 1977 public-domain program by Jim Gillogly
 * Modified for additional speed - 6 December 1988 Phil Karn
 * Modified for parameterized key schedules - Jan 1991 Phil Karn
 * Callers now allocate a key schedule as follows:
 *  kn = (char (*)[8])malloc(sizeof(char) * 8 * 16);
 *  or
 *  char kn[16][8];
 */

/* modified in order to use the libmcrypt API by Nikos Mavroyanopoulos
 * All modifications are placed under the license of libmcrypt.
 */

#include "des.h"

#include <limits> // This needs to be first with GCC 11

#include <qendian.h>
#include <string.h>

static void permute_ip(unsigned char *inblock, DES_KEY *key, unsigned char *outblock);
static void permute_fp(unsigned char *inblock, DES_KEY *key, unsigned char *outblock);
static void perminit_ip(DES_KEY *key);
static void spinit(DES_KEY *key);
static void perminit_fp(DES_KEY *key);
static quint32 f(DES_KEY *key, quint32 r, char *subkey);

/* Tables defined in the Data Encryption Standard documents */

/* initial permutation IP */
static const char ip[] = {58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
                          57, 49, 41, 33, 25, 17, 9,  1, 59, 51, 43, 35, 27, 19, 11, 3, 61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};

/* final permutation IP^-1 */
static const char fp[] = {40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
                          36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27, 34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9,  49, 17, 57, 25};

/* expansion operation matrix
 * This is for reference only; it is unused in the code
 * as the f() function performs it implicitly for speed
 */
#ifdef notdef
static const char ei[] = {32, 1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9,  10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
                          16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};
#endif

/* permuted choice table (key) */
static const char pc1[] = {57, 49, 41, 33, 25, 17, 9,  1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36,

                           63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4};

/* number left rotations of pc1 */
static const char totrot[] = {1, 2, 4, 6, 8, 10, 12, 14, 15, 17, 19, 21, 23, 25, 27, 28};

/* permuted choice key (table) */
static const char pc2[] = {14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,  26, 8,  16, 7,  27, 20, 13, 2,
                           41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};

/* The (in)famous S-boxes */
static const char si[8][64] = {
    /* S1 */
    {14, 4, 13, 1, 2,  15, 11, 8,  3,  10, 6, 12, 5, 9,  0, 7, 0,  15, 7, 4, 14, 2, 13, 1, 10, 6,  12, 11, 9,  5, 3, 8,
     4,  1, 14, 8, 13, 6,  2,  11, 15, 12, 9, 7,  3, 10, 5, 0, 15, 12, 8, 2, 4,  9, 1,  7, 5,  11, 3,  14, 10, 0, 6, 13},

    /* S2 */
    {15, 1,  8, 14, 6,  11, 3,  4, 9, 7, 2,  13, 12, 0, 5, 10, 3,  13, 4,  7, 15, 2,  8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
     0,  14, 7, 11, 10, 4,  13, 1, 5, 8, 12, 6,  9,  3, 2, 15, 13, 8,  10, 1, 3,  15, 4, 2,  11, 6, 7, 12, 0, 5, 14, 9},

    /* S3 */
    {10, 0, 9, 14, 6, 3,  15, 5, 1,  13, 12, 7,  11, 4,  2,  8, 13, 7,  0,  9, 3, 4, 6, 10, 2, 8,  5,  14, 12, 11, 15, 1,
     13, 6, 4, 9,  8, 15, 3,  0, 11, 1,  2,  12, 5,  10, 14, 7, 1,  10, 13, 0, 6, 9, 8, 7,  4, 15, 14, 3,  11, 5,  2,  12},

    /* S4 */
    {7,  13, 14, 3, 0,  6,  9, 10, 1,  2, 8, 5,  11, 12, 4, 15, 13, 8,  11, 5, 6,  15, 0,  3, 4, 7, 2, 12, 1,  10, 14, 9,
     10, 6,  9,  0, 12, 11, 7, 13, 15, 1, 3, 14, 5,  2,  8, 4,  3,  15, 0,  6, 10, 1,  13, 8, 9, 4, 5, 11, 12, 7,  2,  14},

    /* S5 */
    {2, 12, 4, 1,  7,  10, 11, 6, 8,  5, 3,  15, 13, 0, 14, 9,  14, 11, 2,  12, 4, 7,  13, 1,  5, 0,  15, 10, 3,  9, 8, 6,
     4, 2,  1, 11, 10, 13, 7,  8, 15, 9, 12, 5,  6,  3, 0,  14, 11, 8,  12, 7,  1, 14, 2,  13, 6, 15, 0,  9,  10, 4, 5, 3},

    /* S6 */
    {12, 1,  10, 15, 9, 2, 6,  8, 0, 13, 3, 4,  14, 7,  5,  11, 10, 15, 4, 2,  7, 12, 9,  5,  6,  1,  13, 14, 0, 11, 3, 8,
     9,  14, 15, 5,  2, 8, 12, 3, 7, 0,  4, 10, 1,  13, 11, 6,  4,  3,  2, 12, 9, 5,  15, 10, 11, 14, 1,  7,  6, 0,  8, 13},

    /* S7 */
    {4, 11, 2,  14, 15, 0, 8, 13, 3,  12, 9, 7, 5, 10, 6, 1, 13, 0,  11, 7, 4, 9, 1,  10, 14, 3, 5, 12, 2,  15, 8, 6,
     1, 4,  11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5,  9, 2, 6,  11, 13, 8, 1, 4, 10, 7,  9,  5, 0, 15, 14, 2,  3, 12},

    /* S8 */
    {13, 2,  8, 4, 6, 15, 11, 1, 10, 9, 3,  14, 5,  0, 12, 7, 1, 15, 13, 8, 10, 3,  7, 4,  12, 5,  6, 11, 0, 14, 9, 2,
     7,  11, 4, 1, 9, 12, 14, 2, 0,  6, 10, 13, 15, 3, 5,  8, 2, 1,  14, 7, 4,  10, 8, 13, 15, 12, 9, 0,  3, 5,  6, 11},

};

/* 32-bit permutation function P used on the output of the S-boxes */
static const char p32i[] = {16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10, 2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25};

/* End of DES-defined tables */

/* Lookup tables initialized once only at startup by desinit() */

/* bit 0 is left-most in byte */
static const int bytebit[] = {0200, 0100, 040, 020, 010, 04, 02, 01};

static const int nibblebit[] = {010, 04, 02, 01};

/* Allocate space and initialize DES lookup arrays
 * mode == 0: standard Data Encryption Algorithm
 */
static int desinit(DES_KEY *key)
{
    spinit(key);
    perminit_ip(key);
    perminit_fp(key);

    return 0;
}

/* Set key (initialize key schedule array) */
int ntlm_des_set_key(DES_KEY *dkey, char *user_key, int /*len*/)
{
    char pc1m[56]; /* place to modify pc1 into */
    char pcr[56]; /* place to rotate pc1 into */
    int i;
    int j;
    int l;
    int m;

    memset(dkey, 0, sizeof(DES_KEY));
    desinit(dkey);

    /* Clear key schedule */

    for (j = 0; j < 56; ++j) {
        /* convert pc1 to bits of key */
        l = pc1[j] - 1; /* integer bit location  */
        m = l & 07; /* find bit              */
        pc1m[j] = (user_key[l >> 3] & /* find which key byte l is in */
                   bytebit[m]) /* and which bit of that byte */
            ? 1
            : 0; /* and store 1-bit result */
    }
    for (i = 0; i < 16; ++i) {
        /* key chunk for each iteration */
        for (j = 0; j < 56; ++j) { /* rotate pc1 the right amount */
            pcr[j] = pc1m[(l = j + totrot[i]) < (j < 28 ? 28 : 56) ? l : l - 28];
        }
        /* rotate left and right halves independently */
        for (j = 0; j < 48; ++j) {
            /* select bits individually */
            /* check bit that goes to kn[j] */
            if (pcr[pc2[j] - 1]) {
                /* mask it in if it's there */
                l = j % 6;
                dkey->kn[i][j / 6] |= bytebit[l] >> 2;
            }
        }
    }
    return 0;
}

/* In-place encryption of 64-bit block */
static void ntlm_des_encrypt(DES_KEY *key, unsigned char *block)
{
    quint32 left;
    quint32 right;
    char *knp;
    quint32 work[2]; /* Working data storage */

    permute_ip(block, key, (unsigned char *)work); /* Initial Permutation */
    left = qFromBigEndian(work[0]);
    right = qFromBigEndian(work[1]);

    /* Do the 16 rounds.
     * The rounds are numbered from 0 to 15. On even rounds
     * the right half is fed to f() and the result exclusive-ORs
     * the left half; on odd rounds the reverse is done.
     */
    knp = &key->kn[0][0];
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);
    knp += 8;
    left ^= f(key, right, knp);
    knp += 8;
    right ^= f(key, left, knp);

    /* Left/right half swap, plus byte swap if little-endian */
    work[1] = qToBigEndian(left);
    work[0] = qToBigEndian(right);

    permute_fp((unsigned char *)work, key, block); /* Inverse initial permutation */
}

/* Permute inblock with perm */
static void permute_ip(unsigned char *inblock, DES_KEY *key, unsigned char *outblock)
{
    unsigned char *ib;
    unsigned char *ob; /* ptr to input or output block */
    char *p;
    char *q;
    int j;

    /* Clear output block */
    memset(outblock, 0, 8);

    ib = inblock;
    for (j = 0; j < 16; j += 2, ++ib) {
        /* for each input nibble */
        ob = outblock;
        p = key->iperm[j][(*ib >> 4) & 0xf];
        q = key->iperm[j + 1][*ib & 0xf];
        /* and each output byte, OR the masks together */
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
    }
}

/* Permute inblock with perm */
static void permute_fp(unsigned char *inblock, DES_KEY *key, unsigned char *outblock)
{
    unsigned char *ib;
    unsigned char *ob; /* ptr to input or output block */
    char *p;
    char *q;
    int j;

    /* Clear output block */
    memset(outblock, 0, 8);

    ib = inblock;
    for (j = 0; j < 16; j += 2, ++ib) {
        /* for each input nibble */
        ob = outblock;
        p = key->fperm[j][(*ib >> 4) & 0xf];
        q = key->fperm[j + 1][*ib & 0xf];
        /* and each output byte, OR the masks together */
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
        *ob++ |= *p++ | *q++;
    }
}

/* The nonlinear function f(r,k), the heart of DES */
static quint32 f(DES_KEY *key, quint32 r, char *subkey)
{
    quint32 *spp;
    quint32 rval;
    quint32 rt;
    int er;

#ifdef TRACE
    printf("f(%08lx, %02x %02x %02x %02x %02x %02x %02x %02x) = ", r, subkey[0], subkey[1], subkey[2], subkey[3], subkey[4], subkey[5], subkey[6], subkey[7]);
#endif
    /* Run E(R) ^ K through the combined S & P boxes.
     * This code takes advantage of a convenient regularity in
     * E, namely that each group of 6 bits in E(R) feeding
     * a single S-box is a contiguous segment of R.
     */
    subkey += 7;

    /* Compute E(R) for each block of 6 bits, and run thru boxes */
    er = ((int)r << 1) | ((r & 0x80000000) ? 1 : 0);
    spp = &key->sp[7][0];
    rval = spp[(er ^ *subkey--) & 0x3f];
    spp -= 64;
    rt = (quint32)r >> 3;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rval |= spp[((int)rt ^ *subkey--) & 0x3f];
    spp -= 64;
    rt >>= 4;
    rt |= (r & 1) << 5;
    rval |= spp[((int)rt ^ *subkey) & 0x3f];
#ifdef TRACE
    printf(" %08lx\n", rval);
#endif
    return rval;
}

/* initialize a perm array */
static void perminit_ip(DES_KEY *key)
{
    int l;
    int j;
    int k;
    int i;
    int m;

    /* Clear the permutation array */
    memset(key->iperm, 0, 16 * 16 * 8);

    for (i = 0; i < 16; ++i) { /* each input nibble position */
        for (j = 0; j < 16; ++j) { /* each possible input nibble */
            for (k = 0; k < 64; ++k) {
                /* each output bit position */
                l = ip[k] - 1; /* where does this bit come from */
                if ((l >> 2) != i) { /* does it come from input posn? */
                    continue; /* if not, bit k is 0    */
                }
                if (!(j & nibblebit[l & 3])) {
                    continue; /* any such bit in input? */
                }
                m = k & 07; /* which bit is this in the byte */
                key->iperm[i][j][k >> 3] |= bytebit[m];
            }
        }
    }
}

static void perminit_fp(DES_KEY *key)
{
    int l;
    int j;
    int k;
    int i;
    int m;

    /* Clear the permutation array */
    memset(key->fperm, 0, 16 * 16 * 8);

    for (i = 0; i < 16; ++i) { /* each input nibble position */
        for (j = 0; j < 16; ++j) { /* each possible input nibble */
            for (k = 0; k < 64; ++k) {
                /* each output bit position */
                l = fp[k] - 1; /* where does this bit come from */
                if ((l >> 2) != i) { /* does it come from input posn? */
                    continue; /* if not, bit k is 0    */
                }
                if (!(j & nibblebit[l & 3])) {
                    continue; /* any such bit in input? */
                }
                m = k & 07; /* which bit is this in the byte */
                key->fperm[i][j][k >> 3] |= bytebit[m];
            }
        }
    }
}

/* Initialize the lookup table for the combined S and P boxes */
static void spinit(DES_KEY *key)
{
    char pbox[32];
    int p;
    int i;
    int s;
    int j;
    int rowcol;
    quint32 val;

    /* Compute pbox, the inverse of p32i.
     * This is easier to work with
     */
    for (p = 0; p < 32; ++p) {
        for (i = 0; i < 32; ++i) {
            if (p32i[i] - 1 == p) {
                pbox[p] = i;
                break;
            }
        }
    }
    for (s = 0; s < 8; ++s) {
        /* For each S-box */
        for (i = 0; i < 64; ++i) {
            /* For each possible input */
            val = 0;
            /* The row number is formed from the first and last
             * bits; the column number is from the middle 4
             */
            rowcol = (i & 32) | ((i & 1) ? 16 : 0) | ((i >> 1) & 0xf);
            for (j = 0; j < 4; j++) {
                /* For each output bit */
                if (si[s][rowcol] & (8 >> j)) {
                    val |= 1L << (31 - pbox[4 * s + j]);
                }
            }
            key->sp[s][i] = val;
        }
    }
}

int ntlm_des_ecb_encrypt(const void *plaintext, int len, DES_KEY *akey, unsigned char output[8])
{
    int j;
    const unsigned char *plain = (const unsigned char *)plaintext;

    for (j = 0; j < len / 8; ++j) {
        memcpy(&output[j * 8], &plain[j * 8], 8);
        ntlm_des_encrypt(akey, &output[j * 8]);
    }

    if (j == 0 && len != 0) {
        return -1; /* no blocks were encrypted */
    }
    return 0;
}
