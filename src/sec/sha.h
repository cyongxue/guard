/*
 *  sha.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SEC_SHA_H_
#define SRC_SEC_SHA_H_

namespace sec {

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * ! SHA_LONG has to be at least 32 bits wide. If it's wider, then !
 * ! SHA_LONG_LOG2 has to be defined along.                        !
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

#if defined(__LP32__)
#define SHA_LONG unsigned long
#elif defined(__ILP64__)
#define SHA_LONG unsigned long
#define SHA_LONG_LOG2 3
#else
#define SHA_LONG unsigned int
#endif

#define SHA_LBLOCK  16
#define SHA_CBLOCK  (SHA_LBLOCK*4)  /* SHA treats input data as a
             * contiguous array of 32 bit
             * wide big-endian values. */
#define SHA_LAST_BLOCK  (SHA_CBLOCK-8)
#define SHA_DIGEST_LENGTH 20

typedef struct SHAstate_st {
    SHA_LONG h0, h1, h2, h3, h4;
    SHA_LONG Nl, Nh;
    SHA_LONG data[SHA_LBLOCK];
    unsigned int num;
} SHA_CTX;

int SHA1_Init(SHA_CTX *c);
int SHA1_Update(SHA_CTX *c, const void *data, size_t len);
int SHA1_Final(unsigned char *md, SHA_CTX *c);
unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);
void SHA1_Transform(SHA_CTX *c, const unsigned char *data);

}

#endif /* SRC_SEC_SHA_H_ */
