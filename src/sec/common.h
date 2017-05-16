/*
 *  common.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SEC_COMMON_H_
#define SRC_SEC_COMMON_H_

#include <string.h>
#include <string>

namespace sec {

#define SHA_1
#ifndef SHA_LONG_LOG2
#define SHA_LONG_LOG2   2   /* default to 32 bits */
#endif

#define DATA_ORDER_IS_BIG_ENDIAN

#define HASH_LONG               SHA_LONG
#define HASH_LONG_LOG2          SHA_LONG_LOG2
#define HASH_CTX                SHA_CTX
#define HASH_CBLOCK             SHA_CBLOCK
#define HASH_LBLOCK             SHA_LBLOCK
#define HASH_MAKE_STRING(c,s)   do {    \
    unsigned long ll;       \
    ll=(c)->h0; HOST_l2c(ll,(s));   \
    ll=(c)->h1; HOST_l2c(ll,(s));   \
    ll=(c)->h2; HOST_l2c(ll,(s));   \
    ll=(c)->h3; HOST_l2c(ll,(s));   \
    ll=(c)->h4; HOST_l2c(ll,(s));   \
    } while (0)

#if defined(SHA_0)

# define HASH_UPDATE                SHA_Update
# define HASH_TRANSFORM             SHA_Transform
# define HASH_FINAL                 SHA_Final
# define HASH_INIT                  SHA_Init
# define HASH_BLOCK_HOST_ORDER      sha_block_host_order
# define HASH_BLOCK_DATA_ORDER      sha_block_data_order
# define Xupdate(a,ix,ia,ib,ic,id)  (ix=(a)=(ia^ib^ic^id))

void sha_block_host_order (SHA_CTX *c, const void *p, size_t num);
void sha_block_data_order (SHA_CTX *c, const void *p, size_t num);

#elif defined(SHA_1)

# define HASH_UPDATE                SHA1_Update
# define HASH_TRANSFORM             SHA1_Transform
# define HASH_FINAL                 SHA1_Final
# define HASH_INIT                  SHA1_Init
# define HASH_BLOCK_HOST_ORDER      sha1_block_host_order
# define HASH_BLOCK_DATA_ORDER      sha1_block_data_order
# if defined(__MWERKS__) && defined(__MC68K__)
/* Metrowerks for Motorola fails otherwise:-( <appro@fy.chalmers.se> */
#  define Xupdate(a,ix,ia,ib,ic,id) do { (a)=(ia^ib^ic^id);     \
                         ix=(a)=ROTATE((a),1);  \
                    } while (0)
# else
#  define Xupdate(a,ix,ia,ib,ic,id) ( (a)=(ia^ib^ic^id),    \
                      ix=(a)=ROTATE((a),1)  \
                    )
# endif

# ifdef SHA1_ASM
#  if defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__INTEL__)
#   if !defined(B_ENDIAN)
#    define sha1_block_host_order       sha1_block_asm_host_order
#    define DONT_IMPLEMENT_BLOCK_HOST_ORDER
#    define sha1_block_data_order       sha1_block_asm_data_order
#    define DONT_IMPLEMENT_BLOCK_DATA_ORDER
#    define HASH_BLOCK_DATA_ORDER_ALIGNED   sha1_block_asm_data_order
#   endif
#  elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#   define sha1_block_host_order        sha1_block_asm_host_order
#   define DONT_IMPLEMENT_BLOCK_HOST_ORDER
#   define sha1_block_data_order        sha1_block_asm_data_order
#   define DONT_IMPLEMENT_BLOCK_DATA_ORDER
#  endif
# endif
void sha1_block_host_order (SHA_CTX *c, const void *p, size_t num);
void sha1_block_data_order (SHA_CTX *c, const void *p, size_t num);

#else
# error "Either SHA_0 or SHA_1 must be defined."
#endif


#if !defined(DATA_ORDER_IS_BIG_ENDIAN) && !defined(DATA_ORDER_IS_LITTLE_ENDIAN)
#error "DATA_ORDER must be defined!"
#endif

#ifndef HASH_CBLOCK
#error "HASH_CBLOCK must be defined!"
#endif
#ifndef HASH_LONG
#error "HASH_LONG must be defined!"
#endif
#ifndef HASH_CTX
#error "HASH_CTX must be defined!"
#endif

#ifndef HASH_UPDATE
#error "HASH_UPDATE must be defined!"
#endif
#ifndef HASH_TRANSFORM
#error "HASH_TRANSFORM must be defined!"
#endif
#ifndef HASH_FINAL
#error "HASH_FINAL must be defined!"
#endif

#ifndef HASH_BLOCK_HOST_ORDER
#error "HASH_BLOCK_HOST_ORDER must be defined!"
#endif

#if 0
/*
 * Moved below as it's required only if HASH_BLOCK_DATA_ORDER_ALIGNED
 * isn't defined.
 */
#ifndef HASH_BLOCK_DATA_ORDER
#error "HASH_BLOCK_DATA_ORDER must be defined!"
#endif
#endif

#ifndef HASH_LBLOCK
#define HASH_LBLOCK (HASH_CBLOCK/4)
#endif

#ifndef HASH_LONG_LOG2
#define HASH_LONG_LOG2  2
#endif

/*
 * Engage compiler specific rotate intrinsic function if available.
 */
#undef ROTATE
#ifndef PEDANTIC
# if defined(_MSC_VER) || defined(__ICC)
#  define ROTATE(a,n)   _lrotl(a,n)
# elif defined(__MWERKS__)
#  if defined(__POWERPC__)
#   define ROTATE(a,n)  __rlwinm(a,n,0,31)
#  elif defined(__MC68K__)
/* Motorola specific tweak. <appro@fy.chalmers.se> */
#   define ROTATE(a,n)  ( n<24 ? __rol(a,n) : __ror(a,32-n) )
#  else
#   define ROTATE(a,n)  __rol(a,n)
#  endif
# elif defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
/*
 * Some GNU C inline assembler templates. Note that these are
 * rotates by *constant* number of bits! But that's exactly
 * what we need here...
 *              <appro@fy.chalmers.se>
 */
#  if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
#   define ROTATE(a,n)  ({ register unsigned int ret;   \
                asm (           \
                "roll %1,%0"        \
                : "=r"(ret)     \
                : "I"(n), "0"(a)    \
                : "cc");        \
               ret;             \
            })
#  elif defined(__powerpc) || defined(__ppc__) || defined(__powerpc64__)
#   define ROTATE(a,n)  ({ register unsigned int ret;   \
                asm (           \
                "rlwinm %0,%1,%2,0,31"  \
                : "=r"(ret) \
                : "r"(a), "I"(n));  \
               ret;             \
            })
#  endif
# endif
#endif /* PEDANTIC */

#if HASH_LONG_LOG2==2   /* Engage only if sizeof(HASH_LONG)== 4 */
/* A nice byte order reversal from Wei Dai <weidai@eskimo.com> */
#ifdef ROTATE
/* 5 instructions with rotate instruction, else 9 */
#define REVERSE_FETCH32(a,l)    (                   \
        l=*(const HASH_LONG *)(a),                  \
        ((ROTATE(l,8)&0x00FF00FF)|(ROTATE((l&0x00FF00FF),24)))  \
                )
#else
/* 6 instructions with rotate instruction, else 8 */
#define REVERSE_FETCH32(a,l)    (               \
        l=*(const HASH_LONG *)(a),          \
        l=(((l>>8)&0x00FF00FF)|((l&0x00FF00FF)<<8)),    \
        ROTATE(l,16)                    \
                )
/*
 * Originally the middle line started with l=(((l&0xFF00FF00)>>8)|...
 * It's rewritten as above for two reasons:
 *  - RISCs aren't good at long constants and have to explicitely
 *    compose 'em with several (well, usually 2) instructions in a
 *    register before performing the actual operation and (as you
 *    already realized:-) having same constant should inspire the
 *    compiler to permanently allocate the only register for it;
 *  - most modern CPUs have two ALUs, but usually only one has
 *    circuitry for shifts:-( this minor tweak inspires compiler
 *    to schedule shift instructions in a better way...
 *
 *              <appro@fy.chalmers.se>
 */
#endif
#endif

#ifndef ROTATE
#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

/*
 * Make some obvious choices. E.g., HASH_BLOCK_DATA_ORDER_ALIGNED
 * and HASH_BLOCK_HOST_ORDER ought to be the same if input data
 * and host are of the same "endianess". It's possible to mask
 * this with blank #define HASH_BLOCK_DATA_ORDER though...
 *
 *              <appro@fy.chalmers.se>
 */
#if defined(B_ENDIAN)
#  if defined(DATA_ORDER_IS_BIG_ENDIAN)
#    if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED) && HASH_LONG_LOG2==2
#      define HASH_BLOCK_DATA_ORDER_ALIGNED HASH_BLOCK_HOST_ORDER
#    endif
#  endif
#elif defined(L_ENDIAN)
#  if defined(DATA_ORDER_IS_LITTLE_ENDIAN)
#    if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED) && HASH_LONG_LOG2==2
#      define HASH_BLOCK_DATA_ORDER_ALIGNED HASH_BLOCK_HOST_ORDER
#    endif
#  endif
#endif

#if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
#ifndef HASH_BLOCK_DATA_ORDER
#error "HASH_BLOCK_DATA_ORDER must be defined!"
#endif
#endif

#if defined(DATA_ORDER_IS_BIG_ENDIAN)

#ifndef PEDANTIC
# if defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
#  if ((defined(__i386) || defined(__i386__)) && !defined(I386_ONLY)) || \
      (defined(__x86_64) || defined(__x86_64__))
/*
 * This gives ~30-40% performance improvement in SHA-256 compiled
 * with gcc [on P4]. Well, first macro to be frank. We can pull
 * this trick on x86* platforms only, because these CPUs can fetch
 * unaligned data without raising an exception.
 */
#   define HOST_c2l(c,l)    ({ unsigned int r=*((const unsigned int *)(c));    \
                   asm ("bswapl %0":"=r"(r):"0"(r));    \
                   (c)+=4; (l)=r;            })
#   define HOST_l2c(l,c)    ({ unsigned int r=(l);            \
                   asm ("bswapl %0":"=r"(r):"0"(r));    \
                   *((unsigned int *)(c))=r; (c)+=4; r;    })
#  endif
# endif
#endif

#ifndef HOST_c2l
#define HOST_c2l(c,l)    (l =(((unsigned long)(*((c)++)))<<24),        \
             l|=(((unsigned long)(*((c)++)))<<16),        \
             l|=(((unsigned long)(*((c)++)))<< 8),        \
             l|=(((unsigned long)(*((c)++)))    ),        \
             l)
#endif
#define HOST_p_c2l(c,l,n)    {                    \
            switch (n) {                    \
            case 0: l =((unsigned long)(*((c)++)))<<24;    \
            case 1: l|=((unsigned long)(*((c)++)))<<16;    \
            case 2: l|=((unsigned long)(*((c)++)))<< 8;    \
            case 3: l|=((unsigned long)(*((c)++)));        \
                } }
#define HOST_p_c2l_p(c,l,sc,len) {                    \
            switch (sc) {                    \
            case 0: l =((unsigned long)(*((c)++)))<<24;    \
                if (--len == 0) break;            \
            case 1: l|=((unsigned long)(*((c)++)))<<16;    \
                if (--len == 0) break;            \
            case 2: l|=((unsigned long)(*((c)++)))<< 8;    \
                } }
/* NOTE the pointer is not incremented at the end of this */
#define HOST_c2l_p(c,l,n)    {                    \
            l=0; (c)+=n;                    \
            switch (n) {                    \
            case 3: l =((unsigned long)(*(--(c))))<< 8;    \
            case 2: l|=((unsigned long)(*(--(c))))<<16;    \
            case 1: l|=((unsigned long)(*(--(c))))<<24;    \
                } }
#ifndef HOST_l2c
#define HOST_l2c(l,c)    (*((c)++)=(unsigned char)(((l)>>24)&0xff),    \
             *((c)++)=(unsigned char)(((l)>>16)&0xff),    \
             *((c)++)=(unsigned char)(((l)>> 8)&0xff),    \
             *((c)++)=(unsigned char)(((l)    )&0xff),    \
             l)
#endif

#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)

#if defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__)
# ifndef B_ENDIAN
/* See comment in DATA_ORDER_IS_BIG_ENDIAN section. */
#  define HOST_c2l(c,l)    ((l)=*((const unsigned int *)(c)), (c)+=4, l)
#  define HOST_l2c(l,c)    (*((unsigned int *)(c))=(l), (c)+=4, l)
# endif
#endif

#ifndef HOST_c2l
#define HOST_c2l(c,l)    (l =(((unsigned long)(*((c)++)))    ),        \
             l|=(((unsigned long)(*((c)++)))<< 8),        \
             l|=(((unsigned long)(*((c)++)))<<16),        \
             l|=(((unsigned long)(*((c)++)))<<24),        \
             l)
#endif
#define HOST_p_c2l(c,l,n)    {                    \
            switch (n) {                    \
            case 0: l =((unsigned long)(*((c)++)));        \
            case 1: l|=((unsigned long)(*((c)++)))<< 8;    \
            case 2: l|=((unsigned long)(*((c)++)))<<16;    \
            case 3: l|=((unsigned long)(*((c)++)))<<24;    \
                } }
#define HOST_p_c2l_p(c,l,sc,len) {                    \
            switch (sc) {                    \
            case 0: l =((unsigned long)(*((c)++)));        \
                if (--len == 0) break;            \
            case 1: l|=((unsigned long)(*((c)++)))<< 8;    \
                if (--len == 0) break;            \
            case 2: l|=((unsigned long)(*((c)++)))<<16;    \
                } }
/* NOTE the pointer is not incremented at the end of this */
#define HOST_c2l_p(c,l,n)    {                    \
            l=0; (c)+=n;                    \
            switch (n) {                    \
            case 3: l =((unsigned long)(*(--(c))))<<16;    \
            case 2: l|=((unsigned long)(*(--(c))))<< 8;    \
            case 1: l|=((unsigned long)(*(--(c))));        \
                } }
#ifndef HOST_l2c
#define HOST_l2c(l,c)    (*((c)++)=(unsigned char)(((l)    )&0xff),    \
             *((c)++)=(unsigned char)(((l)>> 8)&0xff),    \
             *((c)++)=(unsigned char)(((l)>>16)&0xff),    \
             *((c)++)=(unsigned char)(((l)>>24)&0xff),    \
             l)
#endif

#endif

enum SecFileType {
	BINARY = 0,
	TEXT = 1
};

}

#endif /* SRC_SEC_COMMON_H_ */
