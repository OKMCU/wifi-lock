#include "md5.h"
#include <string.h>

/*
 *  RFC 1321 compliant MD5 implementation
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct
{
    uint32_t total[2];
    uint32_t state[4];
    uint8_t buffer[64];
}
md5_context;


static uint32_t GetINT32U( const uint8_t *x )
{
  return ( ( (uint32_t)x[0]       )
          |( (uint32_t)x[1] << 8  )
          |( (uint32_t)x[2] << 16 )
          |( (uint32_t)x[3] << 24 )
          );
}
#define GET_UINT32(n,b,i)                       \
{                                               \
    (n) = ( (uint32_t) (b)[(i)    ]       )       \
        | ( (uint32_t) (b)[(i) + 1] <<  8 )       \
        | ( (uint32_t) (b)[(i) + 2] << 16 )       \
        | ( (uint32_t) (b)[(i) + 3] << 24 );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
    (b)[(i)    ] = (uint8_t) ( (n)       );       \
    (b)[(i) + 1] = (uint8_t) ( (n) >>  8 );       \
    (b)[(i) + 2] = (uint8_t) ( (n) >> 16 );       \
    (b)[(i) + 3] = (uint8_t) ( (n) >> 24 );       \
}

static void md5_starts( md5_context *ctx )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

static void md5_process( md5_context *ctx, const uint8_t data[64] )
{
    uint32_t X[16], cVarA, cVarB, cVarC, cVarD;
    uint8_t i;
    for( i = 0 ; i < 16 ; i++ )
    {
      X[i] = GetINT32U( data + i * 4 );
    }

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

    cVarA = ctx->state[0];
    cVarB = ctx->state[1];
    cVarC = ctx->state[2];
    cVarD = ctx->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

    P( cVarA, cVarB, cVarC, cVarD,  0,  7, 0xD76AA478 );
    P( cVarD, cVarA, cVarB, cVarC,  1, 12, 0xE8C7B756 );
    P( cVarC, cVarD, cVarA, cVarB,  2, 17, 0x242070DB );
    P( cVarB, cVarC, cVarD, cVarA,  3, 22, 0xC1BDCEEE );
    P( cVarA, cVarB, cVarC, cVarD,  4,  7, 0xF57C0FAF );
    P( cVarD, cVarA, cVarB, cVarC,  5, 12, 0x4787C62A );
    P( cVarC, cVarD, cVarA, cVarB,  6, 17, 0xA8304613 );
    P( cVarB, cVarC, cVarD, cVarA,  7, 22, 0xFD469501 );
    P( cVarA, cVarB, cVarC, cVarD,  8,  7, 0x698098D8 );
    P( cVarD, cVarA, cVarB, cVarC,  9, 12, 0x8B44F7AF );
    P( cVarC, cVarD, cVarA, cVarB, 10, 17, 0xFFFF5BB1 );
    P( cVarB, cVarC, cVarD, cVarA, 11, 22, 0x895CD7BE );
    P( cVarA, cVarB, cVarC, cVarD, 12,  7, 0x6B901122 );
    P( cVarD, cVarA, cVarB, cVarC, 13, 12, 0xFD987193 );
    P( cVarC, cVarD, cVarA, cVarB, 14, 17, 0xA679438E );
    P( cVarB, cVarC, cVarD, cVarA, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

    P( cVarA, cVarB, cVarC, cVarD,  1,  5, 0xF61E2562 );
    P( cVarD, cVarA, cVarB, cVarC,  6,  9, 0xC040B340 );
    P( cVarC, cVarD, cVarA, cVarB, 11, 14, 0x265E5A51 );
    P( cVarB, cVarC, cVarD, cVarA,  0, 20, 0xE9B6C7AA );
    P( cVarA, cVarB, cVarC, cVarD,  5,  5, 0xD62F105D );
    P( cVarD, cVarA, cVarB, cVarC, 10,  9, 0x02441453 );
    P( cVarC, cVarD, cVarA, cVarB, 15, 14, 0xD8A1E681 );
    P( cVarB, cVarC, cVarD, cVarA,  4, 20, 0xE7D3FBC8 );
    P( cVarA, cVarB, cVarC, cVarD,  9,  5, 0x21E1CDE6 );
    P( cVarD, cVarA, cVarB, cVarC, 14,  9, 0xC33707D6 );
    P( cVarC, cVarD, cVarA, cVarB,  3, 14, 0xF4D50D87 );
    P( cVarB, cVarC, cVarD, cVarA,  8, 20, 0x455A14ED );
    P( cVarA, cVarB, cVarC, cVarD, 13,  5, 0xA9E3E905 );
    P( cVarD, cVarA, cVarB, cVarC,  2,  9, 0xFCEFA3F8 );
    P( cVarC, cVarD, cVarA, cVarB,  7, 14, 0x676F02D9 );
    P( cVarB, cVarC, cVarD, cVarA, 12, 20, 0x8D2A4C8A );

#undef F
    
#define F(x,y,z) (x ^ y ^ z)

    P( cVarA, cVarB, cVarC, cVarD,  5,  4, 0xFFFA3942 );
    P( cVarD, cVarA, cVarB, cVarC,  8, 11, 0x8771F681 );
    P( cVarC, cVarD, cVarA, cVarB, 11, 16, 0x6D9D6122 );
    P( cVarB, cVarC, cVarD, cVarA, 14, 23, 0xFDE5380C );
    P( cVarA, cVarB, cVarC, cVarD,  1,  4, 0xA4BEEA44 );
    P( cVarD, cVarA, cVarB, cVarC,  4, 11, 0x4BDECFA9 );
    P( cVarC, cVarD, cVarA, cVarB,  7, 16, 0xF6BB4B60 );
    P( cVarB, cVarC, cVarD, cVarA, 10, 23, 0xBEBFBC70 );
    P( cVarA, cVarB, cVarC, cVarD, 13,  4, 0x289B7EC6 );
    P( cVarD, cVarA, cVarB, cVarC,  0, 11, 0xEAA127FA );
    P( cVarC, cVarD, cVarA, cVarB,  3, 16, 0xD4EF3085 );
    P( cVarB, cVarC, cVarD, cVarA,  6, 23, 0x04881D05 );
    P( cVarA, cVarB, cVarC, cVarD,  9,  4, 0xD9D4D039 );
    P( cVarD, cVarA, cVarB, cVarC, 12, 11, 0xE6DB99E5 );
    P( cVarC, cVarD, cVarA, cVarB, 15, 16, 0x1FA27CF8 );
    P( cVarB, cVarC, cVarD, cVarA,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) (y ^ (x | ~z))

    P( cVarA, cVarB, cVarC, cVarD,  0,  6, 0xF4292244 );
    P( cVarD, cVarA, cVarB, cVarC,  7, 10, 0x432AFF97 );
    P( cVarC, cVarD, cVarA, cVarB, 14, 15, 0xAB9423A7 );
    P( cVarB, cVarC, cVarD, cVarA,  5, 21, 0xFC93A039 );
    P( cVarA, cVarB, cVarC, cVarD, 12,  6, 0x655B59C3 );
    P( cVarD, cVarA, cVarB, cVarC,  3, 10, 0x8F0CCC92 );
    P( cVarC, cVarD, cVarA, cVarB, 10, 15, 0xFFEFF47D );
    P( cVarB, cVarC, cVarD, cVarA,  1, 21, 0x85845DD1 );
    P( cVarA, cVarB, cVarC, cVarD,  8,  6, 0x6FA87E4F );
    P( cVarD, cVarA, cVarB, cVarC, 15, 10, 0xFE2CE6E0 );
    P( cVarC, cVarD, cVarA, cVarB,  6, 15, 0xA3014314 );
    P( cVarB, cVarC, cVarD, cVarA, 13, 21, 0x4E0811A1 );
    P( cVarA, cVarB, cVarC, cVarD,  4,  6, 0xF7537E82 );
    P( cVarD, cVarA, cVarB, cVarC, 11, 10, 0xBD3AF235 );
    P( cVarC, cVarD, cVarA, cVarB,  2, 15, 0x2AD7D2BB );
    P( cVarB, cVarC, cVarD, cVarA,  9, 21, 0xEB86D391 );

#undef F

    ctx->state[0] += cVarA;
    ctx->state[1] += cVarB;
    ctx->state[2] += cVarC;
    ctx->state[3] += cVarD;
}

static void md5_update( md5_context *ctx, const uint8_t *input, uint32_t length )
{
    uint32_t left, fill;

    if( ! length ) return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += length;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < length )
        ctx->total[1]++;

    if( left && length >= fill )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, fill );
        md5_process( ctx, ctx->buffer );
        length -= fill;
        input  += fill;
        left = 0;
    }

    while( length >= 64 )
    {
        md5_process( ctx, input );
        length -= 64;
        input  += 64;
    }

    if( length )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, length );
    }
}

static const uint8_t md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void md5_finish( md5_context *ctx, uint8_t digest[MD5_SIZE] )
{
    uint32_t last, padn;
    uint32_t high, low;
    uint8_t msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32( low,  msglen, 0 );
    PUT_UINT32( high, msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    md5_update( ctx, (uint8_t *)md5_padding, padn );
    md5_update( ctx, msglen, 8 );

    PUT_UINT32( ctx->state[0], digest,  0 );
    PUT_UINT32( ctx->state[1], digest,  4 );
    PUT_UINT32( ctx->state[2], digest,  8 );
    PUT_UINT32( ctx->state[3], digest, 12 );
}

extern void md5( uint8_t md5val[MD5_SIZE] , const uint8_t *src , uint32_t len )
{
  md5_context ctx;
  md5_starts( &ctx );
  md5_update( &ctx, src, len );
  md5_finish( &ctx, md5val );
}
