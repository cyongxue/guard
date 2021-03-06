/*
 *  md32_common.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

/*
 * This is a generic 32 bit "collector" for message digest algorithms.
 * Whenever needed it collects input character stream into chunks of
 * 32 bit values and invokes a block function that performs actual hash
 * calculations.
 *
 * Porting guide.
 *
 * Obligatory macros:
 *
 * DATA_ORDER_IS_BIG_ENDIAN or DATA_ORDER_IS_LITTLE_ENDIAN
 *  this macro defines byte order of input stream.
 * HASH_CBLOCK
 *  size of a unit chunk HASH_BLOCK operates on.
 * HASH_LONG
 *  has to be at lest 32 bit wide, if it's wider, then
 *  HASH_LONG_LOG2 *has to* be defined along
 * HASH_CTX
 *  context structure that at least contains following
 *  members:
 *      typedef struct {
 *          ...
 *          HASH_LONG       Nl,Nh;
 *          HASH_LONG       data[HASH_LBLOCK];
 *          unsigned int    num;
 *          ...
 *          } HASH_CTX;
 * HASH_UPDATE
 *  name of "Update" function, implemented here.
 * HASH_TRANSFORM
 *  name of "Transform" function, implemented here.
 * HASH_FINAL
 *  name of "Final" function, implemented here.
 * HASH_BLOCK_HOST_ORDER
 *  name of "block" function treating *aligned* input message
 *  in host byte order, implemented externally.
 * HASH_BLOCK_DATA_ORDER
 *  name of "block" function treating *unaligned* input message
 *  in original (data) byte order, implemented externally (it
 *  actually is optional if data and host are of the same
 *  "endianess").
 * HASH_MAKE_STRING
 *  macro convering context variables to an ASCII hash string.
 *
 * Optional macros:
 *
 * B_ENDIAN or L_ENDIAN
 *  defines host byte-order.
 * HASH_LONG_LOG2
 *  defaults to 2 if not states otherwise.
 * HASH_LBLOCK
 *  assumed to be HASH_CBLOCK/4 if not stated otherwise.
 * HASH_BLOCK_DATA_ORDER_ALIGNED
 *  alternative "block" function capable of treating
 *  aligned input message in original (data) order,
 *  implemented externally.
 *
 * MD5 example:
 *
 *  #define DATA_ORDER_IS_LITTLE_ENDIAN
 *
 *  #define HASH_LONG       MD5_LONG
 *  #define HASH_LONG_LOG2  MD5_LONG_LOG2
 *  #define HASH_CTX        MD5_CTX
 *  #define HASH_CBLOCK     MD5_CBLOCK
 *  #define HASH_LBLOCK     MD5_LBLOCK
 *  #define HASH_UPDATE     MD5_Update
 *  #define HASH_TRANSFORM  MD5_Transform
 *  #define HASH_FINAL      MD5_Final
 *  #define HASH_BLOCK_HOST_ORDER   md5_block_host_order
 *  #define HASH_BLOCK_DATA_ORDER   md5_block_data_order
 *
 *              <appro@fy.chalmers.se>
 */
#ifndef SRC_SEC_MD32_COMMON_H_
#define SRC_SEC_MD32_COMMON_H_

#include "common.h"

namespace sec {

/*
 * Time for some action:-)
 */

int HASH_UPDATE (HASH_CTX *c, const void *data_, size_t len)
{
    const unsigned char *data = (const unsigned char *)data_;
    register HASH_LONG *p;
    register HASH_LONG l;
    size_t sw, sc, ew, ec;

    if (len == 0) {
        return 1;
    }

    l = (c->Nl + (((HASH_LONG)len) << 3)) & 0xffffffffUL;
    /* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
     * Wei Dai <weidai@eskimo.com> for pointing it out. */
    if (l < c->Nl) { /* overflow */
        c->Nh++;
    }
    c->Nh += (len >> 29);    /* might cause compiler warning on 16-bit */
    c->Nl = l;

    if (c->num != 0) {
        p = c->data;
        sw = c->num >> 2;
        sc = c->num & 0x03;

        if ((c->num + len) >= HASH_CBLOCK) {
            l = p[sw];
            HOST_p_c2l(data, l, sc);
            p[sw++] = l;
            for (; sw < HASH_LBLOCK; sw++) {
                HOST_c2l(data, l);
                p[sw] = l;
            }
            HASH_BLOCK_HOST_ORDER (c, p, 1);
            len -= (HASH_CBLOCK - c->num);
            c->num = 0;
            /* drop through and do the rest */
        } else {
            c->num += (unsigned int)len;
            if ((sc + len) < 4) { /* ugly, add char's to a word */
                l = p[sw];
                HOST_p_c2l_p(data, l, sc, len);
                p[sw] = l;
            } else {
                ew = (c->num >> 2);
                ec = (c->num & 0x03);
                if (sc) {
                    l = p[sw];
                }
                HOST_p_c2l(data, l, sc);
                p[sw++] = l;
                for (; sw < ew; sw++) {
                    HOST_c2l(data, l);
                    p[sw] = l;
                }
                if (ec) {
                    HOST_c2l_p(data, l, ec);
                    p[sw] = l;
                }
            }
            return 1;
        }
    }

    sw = len / HASH_CBLOCK;
    if (sw > 0) {
#if defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
        /*
         * Note that HASH_BLOCK_DATA_ORDER_ALIGNED gets defined
         * only if sizeof(HASH_LONG)==4.
         */
        if ((((size_t)data) % 4) == 0) {
            /* data is properly aligned so that we can cast it: */
            HASH_BLOCK_DATA_ORDER_ALIGNED (c, (const HASH_LONG *)data, sw);
            sw *= HASH_CBLOCK;
            data += sw;
            len -= sw;
        } else
#if !defined(HASH_BLOCK_DATA_ORDER)
            while (sw--) {
                memcpy (p = c->data, data, HASH_CBLOCK);
                HASH_BLOCK_DATA_ORDER_ALIGNED(c, p, 1);
                data += HASH_CBLOCK;
                len -= HASH_CBLOCK;
            }
#endif
#endif
#if defined(HASH_BLOCK_DATA_ORDER)
        {
            HASH_BLOCK_DATA_ORDER(c, data, sw);
            sw *= HASH_CBLOCK;
            data += sw;
            len -= sw;
        }
#endif
    }

    if (len != 0) {
        p = c->data;
        c->num = len;
        ew = len >> 2;    /* words to copy */
        ec = len & 0x03;
        for (; ew; ew--, p++) {
            HOST_c2l(data, l);
            *p = l;
        }
        HOST_c2l_p(data, l, ec);
        *p = l;
    }
    return 1;
}


void HASH_TRANSFORM (HASH_CTX *c, const unsigned char *data)
{
#if defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
    if ((((size_t)data) % 4) == 0)
        /* data is properly aligned so that we can cast it: */
    {
        HASH_BLOCK_DATA_ORDER_ALIGNED (c, (const HASH_LONG *)data, 1);
    } else
#if !defined(HASH_BLOCK_DATA_ORDER)
    {
        memcpy (c->data, data, HASH_CBLOCK);
        HASH_BLOCK_DATA_ORDER_ALIGNED (c, c->data, 1);
    }
#endif
#endif
#if defined(HASH_BLOCK_DATA_ORDER)
    HASH_BLOCK_DATA_ORDER (c, data, 1);
#endif
}


int HASH_FINAL (unsigned char *md, HASH_CTX *c)
{
    register HASH_LONG *p;
    register unsigned long l;
    register int i, j;
    static const unsigned char end[4] = {0x80, 0x00, 0x00, 0x00};
    const unsigned char *cp = end;

    /* c->num should definitly have room for at least one more byte. */
    p = c->data;
    i = c->num >> 2;
    j = c->num & 0x03;

#if 0
    /* purify often complains about the following line as an
     * Uninitialized Memory Read.  While this can be true, the
     * following p_c2l macro will reset l when that case is true.
     * This is because j&0x03 contains the number of 'valid' bytes
     * already in p[i].  If and only if j&0x03 == 0, the UMR will
     * occur but this is also the only time p_c2l will do
     * l= *(cp++) instead of l|= *(cp++)
     * Many thanks to Alex Tang <altitude@cic.net> for pickup this
     * 'potential bug' */
#ifdef PURIFY
    if (j == 0) {
        p[i] = 0;    /* Yeah, but that's not the way to fix it:-) */
    }
#endif
    l = p[i];
#else
    l = (j == 0) ? 0 : p[i];
#endif
    HOST_p_c2l(cp, l, j);
    p[i++] = l; /* i is the next 'undefined word' */

    if (i > (HASH_LBLOCK - 2)) { /* save room for Nl and Nh */
        if (i < HASH_LBLOCK) {
            p[i] = 0;
        }
        HASH_BLOCK_HOST_ORDER (c, p, 1);
        i = 0;
    }
    for (; i < (HASH_LBLOCK - 2); i++) {
        p[i] = 0;
    }

#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
    p[HASH_LBLOCK - 2] = c->Nh;
    p[HASH_LBLOCK - 1] = c->Nl;
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
    p[HASH_LBLOCK - 2] = c->Nl;
    p[HASH_LBLOCK - 1] = c->Nh;
#endif
    HASH_BLOCK_HOST_ORDER (c, p, 1);

#ifndef HASH_MAKE_STRING
#error "HASH_MAKE_STRING must be defined!"
#else
    HASH_MAKE_STRING(c, md);
#endif

    c->num = 0;
    /* clear stuff, HASH_BLOCK may be leaving some stuff on the stack
     * but I'm not worried :-)
    OPENSSL_cleanse((void *)c,sizeof(HASH_CTX));
     */
    return 1;
}

#ifndef MD32_REG_T
#define MD32_REG_T int
/*
 * This comment was originaly written for MD5, which is why it
 * discusses A-D. But it basically applies to all 32-bit digests,
 * which is why it was moved to common header file.
 *
 * In case you wonder why A-D are declared as long and not
 * as MD5_LONG. Doing so results in slight performance
 * boost on LP64 architectures. The catch is we don't
 * really care if 32 MSBs of a 64-bit register get polluted
 * with eventual overflows as we *save* only 32 LSBs in
 * *either* case. Now declaring 'em long excuses the compiler
 * from keeping 32 MSBs zeroed resulting in 13% performance
 * improvement under SPARC Solaris7/64 and 5% under AlphaLinux.
 * Well, to be honest it should say that this *prevents*
 * performance degradation.
 *                <appro@fy.chalmers.se>
 * Apparently there're LP64 compilers that generate better
 * code if A-D are declared int. Most notably GCC-x86_64
 * generates better code.
 *                <appro@fy.chalmers.se>
 */
#endif
}

#endif /* SRC_SEC_MD32_COMMON_H_ */
