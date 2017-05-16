/*
 *  md5.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年3月24日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SEC_MD5_H_
#define SRC_SEC_MD5_H_

#include <sys/types.h>
#if defined SOLARIS
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t u_int8_t;
#endif

#if defined HPUX
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t u_int8_t;
#endif

#ifdef WIN32
typedef unsigned int u_int32_t;
#endif

typedef u_int32_t uint32;

struct MD5Context {
    uint32 buf[4];
    uint32 bits[2];
    union {
        unsigned char in[64];
        uint32 in32[16];
    };
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
               unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

/* This is needed to make RSAREF happy on some MS-DOS compilers */
typedef struct MD5Context MD5_CTX;

#endif /* SRC_SEC_MD5_H_ */
