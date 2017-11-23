/*
* Author: Christian Huitema
* Copyright (c) 2017, Private Octopus, Inc.
* All rights reserved.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Private Octopus, Inc. BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "../picoquic/picoquic_internal.h"

static const uint64_t test_number[] = {
    0,
    1,
    0xFFFFFFFFFFFFFFFFull,
    0xDEADBEEFull,
    0x12345678DEADBEEFull
};

static size_t nb_test_numbers = sizeof(test_number) / sizeof(const uint64_t);

static uint64_t decode_number(uint8_t * bytes, size_t length)
{
    uint64_t n = 0;

    for (size_t i = 0; i < length; i++)
    {
        n <<= 8;

        n += bytes[i];
    }

    return n;
}

int intformattest()
{
    /* Test the formating routines */
    int ret = 0;
    uint8_t bytes[8];
    uint64_t decoded;
    uint64_t parsed;
    uint32_t test32;
    uint16_t test16;
    uint64_t test64;

    /* First test with 16 bits macros */
    for (size_t i = 0; ret == 0 && i < nb_test_numbers; i++)
    {
        test16 = (uint16_t)test_number[i];
        picoformat_16(bytes, test16);
        decoded = decode_number(bytes, 2);
        if (decoded != test16)
        {
            ret = -1;
        }
        else
        {
            parsed = PICOPARSE_16(bytes);
            if (parsed != test16)
            {
                ret = -1;
            }
        }
    }

    /* Next test with 32 bits macros */
    for (size_t i = 0; ret == 0 && i < nb_test_numbers; i++)
    {
        test32 = (uint32_t)test_number[i];
        picoformat_32(bytes, test32);
        decoded = decode_number(bytes, 4);
        if (decoded != test32)
        {
            ret = -1;
        }
        else
        {
            parsed = PICOPARSE_32(bytes);
            if (parsed != test32)
            {
                ret = -1;
            }
        }
    }

    /* Final test with 64 bits macros */
    for (size_t i = 0; ret == 0 && i < nb_test_numbers; i++)
    {
        test64 = test_number[i];
        picoformat_64(bytes, test64);
        decoded = decode_number(bytes, 8);
        if (decoded != test64)
        {
            ret = -1;
        }
        else
        {
            parsed = PICOPARSE_64(bytes);
            if (parsed != test64)
            {
                ret = -1;
            }
        }
    }

    return ret;
}

typedef struct st_picoquic_varintformat_test_t {
    uint8_t encoding[8];
    size_t length;
    uint64_t decoded;
    int is_canonical;
} picoquic_varintformat_test_t;

static picoquic_varintformat_test_t varint_test_cases[] = {
    {
        {0, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        1,
        0,
        1
    },
    {
        { 1, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        1,
        1,
        1
    },
    {
        { 63, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        1,
        63,
        1
    },
    {
        { 0x40, 64, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        2,
        64,
        1
    },
    {
        { 0x7F, 0xFF, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        2,
        0x3FFF,
        1
    },
    {
        { 0x80, 0, 0x40, 0, 0xCC, 0xCC, 0xCC, 0xCC },
        4,
        0x4000,
        1
    },
    {
        { 0xBF, 0xFF, 0xFF, 0xFF, 0xCC, 0xCC, 0xCC, 0xCC },
        4,
        0x3FFFFFFF,
        1
    },
    {
        { 0xC0, 0, 0, 0, 0x40, 0, 0, 0 },
        8,
        0x40000000,
        1
    },
    {
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        8,
        0x3FFFFFFFFFFFFFFFull,
        1
    },
    /* For example, the eight octet sequence c2 19 7c 5e ff 14 e8 8c (in hexadecimal) 
     * decodes to the decimal value 151288809941952652; 
     * the four octet sequence 9d 7f 3e 7d decodes to 494878333; 
     * the two octet sequence 7b bd decodes to 15293; 
     * and the single octet 25 decodes to 37 (as does the two octet sequence 40 25). */
    {
        { 0xc2, 0x19, 0x7c, 0x5e, 0xff, 0x14, 0xe8, 0x8c },
        8,
        151288809941952652ull,
        1
    },
    {
        { 0x9d, 0x7f, 0x3e, 0x7d, 0xff, 0x14, 0xe8, 0x8c },
        4,
        494878333,
        1
    },
    {
        { 0xC0, 0, 0, 0, 0x1d, 0x7f, 0x3e, 0x7d },
        8,
        494878333,
        0
    },
    {
        { 0x7b, 0xbd, 0x3e, 0x7d, 0xff, 0x14, 0xe8, 0x8c },
        2,
        15293,
        1
    },
    {
        { 0x80, 0, 0x3b, 0xbd, 0x3e, 0x7d, 0xff, 0x14 },
        4,
        15293,
        0
    },
    {
        { 0xC0, 0, 0, 0, 0, 0, 0x3b, 0xbd },
        8,
        15293,
        0
    },
    {
        { 0x25, 0xbd, 0x3e, 0x7d, 0xff, 0x14, 0xe8, 0x8c },
        1,
        37,
        1
    },
    {
        { 0x40, 0x25, 0xbd, 0x3e, 0x7d, 0xff, 0x14, 0xe8 },
        2,
        37,
        0
    }
};

static size_t nb_varint_test_cases = sizeof(varint_test_cases) / sizeof(picoquic_varintformat_test_t);

int varint_test()
{
    int ret = 0;

    for (size_t i = 0; i < nb_varint_test_cases; i++)
    {
        uint64_t n64;
        size_t length = picoquic_varint_decode(
            varint_test_cases[i].encoding, 8, &n64);

        if (length != varint_test_cases[i].length)
        {
            ret = -1;
        }
        else if (n64 != varint_test_cases[i].decoded)
        {
            ret = -1;
        }
        else if (varint_test_cases[i].is_canonical != 0)
        {
            uint8_t encoding[8];
            size_t coded_length = picoquic_varint_encode(
                encoding,
                varint_test_cases[i].length,
                n64);

            if (coded_length != varint_test_cases[i].length)
            {
                ret = -1;
            }
            else if (memcmp(encoding,
                varint_test_cases[i].encoding,
                coded_length) != 0)
            {
                ret = -1;
            }
        }
    }

    return ret;
}